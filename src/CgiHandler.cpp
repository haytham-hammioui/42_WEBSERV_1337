#include "CgiHandler.hpp"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <stdexcept>
#include <sstream>

#define CGI_BUF_SIZE 4096

CgiHandler::CgiHandler() {}
CgiHandler::CgiHandler(const CgiHandler& other) { (void)other; }
CgiHandler& CgiHandler::operator=(const CgiHandler& other) { (void)other; return *this; }
CgiHandler::~CgiHandler() {}

// ─── ENV BUILDING ─────────────────────────────────────────────────────────────

char** CgiHandler::buildEnv(const std::map<std::string,std::string>& env_vars) {
    // allocate array of char* with one extra slot for the NULL terminator
    char** env = new char*[env_vars.size() + 1];
    size_t i   = 0;

    std::map<std::string,std::string>::const_iterator it;
    for (it = env_vars.begin(); it != env_vars.end(); ++it) {
        std::string entry = it->first + "=" + it->second;
        env[i] = new char[entry.size() + 1];
        std::strcpy(env[i], entry.c_str());
        i++;
    }
    env[i] = NULL;  // execve requires NULL-terminated array
    return env;
}

void CgiHandler::freeEnv(char** env, size_t count) {
    for (size_t i = 0; i < count; i++)
        delete[] env[i];
    delete[] env;
}

// ─── CGI OUTPUT PARSING ───────────────────────────────────────────────────────

void CgiHandler::parseCgiOutput(const std::string& raw,
                                 std::string&       headers,
                                 std::string&       body) {
    // CGI output: headers, blank line, body
    // blank line = \r\n\r\n  or  \n\n
    size_t sep = raw.find("\r\n\r\n");
    if (sep != std::string::npos) {
        headers = raw.substr(0, sep);
        body    = raw.substr(sep + 4);
        return;
    }
    sep = raw.find("\n\n");
    if (sep != std::string::npos) {
        headers = raw.substr(0, sep);
        body    = raw.substr(sep + 2);
        return;
    }
    // no separator found — treat everything as body
    headers = "";
    body    = raw;
}

// ─── MAIN EXECUTE ─────────────────────────────────────────────────────────────

CgiResult CgiHandler::execute(const std::string&                    interpreter,
                               const std::string&                    script_path,
                               const std::string&                    request_body,
                               const std::map<std::string,std::string>& env_vars,
                               int                                   timeout_sec) {
    CgiResult result;

    // build env before forking (cleaner to do it in the parent)
    char** env = buildEnv(env_vars);

    // two pipes:
    //   input_pipe:  parent writes request body → child reads as stdin
    //   output_pipe: child writes response → parent reads
    int input_pipe[2];
    int output_pipe[2];

    if (pipe(input_pipe) == -1 || pipe(output_pipe) == -1) {
        freeEnv(env, env_vars.size());
        result.error       = true;
        result.status_code = 500;
        return result;
    }

    pid_t pid = fork();

    if (pid == -1) {
        // fork failed
        close(input_pipe[0]);  close(input_pipe[1]);
        close(output_pipe[0]); close(output_pipe[1]);
        freeEnv(env, env_vars.size());
        result.error       = true;
        result.status_code = 500;
        return result;
    }

    // ── CHILD ──────────────────────────────────────────────────────────────
    if (pid == 0) {
        // redirect stdin from input_pipe's read end
        if (dup2(input_pipe[0], STDIN_FILENO) == -1)
            exit(1);

        // redirect stdout to output_pipe's write end
        if (dup2(output_pipe[1], STDOUT_FILENO) == -1)
            exit(1);

        // close all pipe ends we no longer need
        close(input_pipe[0]);
        close(input_pipe[1]);
        close(output_pipe[0]);
        close(output_pipe[1]);

        // move into the script's directory (subject requirement)
        std::string script_dir = script_path;
        size_t slash = script_dir.rfind('/');
        if (slash != std::string::npos)
            script_dir = script_dir.substr(0, slash);
        if (chdir(script_dir.c_str()) == -1)
            exit(1);

        // build execve arguments
        char* args[3];
        args[0] = const_cast<char*>(interpreter.c_str());
        args[1] = const_cast<char*>(script_path.c_str());
        args[2] = NULL;

        execve(interpreter.c_str(), args, env);

        // execve only reaches here if it failed
        exit(1);
    }

    // ── PARENT ─────────────────────────────────────────────────────────────
    // close the ends we don't use
    close(input_pipe[0]);   // child reads this; parent doesn't
    close(output_pipe[1]);  // child writes this; parent doesn't

    // write request body to child's stdin
    if (!request_body.empty()) {
        const char* ptr  = request_body.c_str();
        size_t      left = request_body.size();
        while (left > 0) {
            ssize_t written = write(input_pipe[1], ptr, left);
            if (written <= 0) break;
            ptr  += written;
            left -= written;
        }
    }
    // closing write end sends EOF to the child's stdin
    close(input_pipe[1]);

    // make the output pipe non-blocking so we never block in read()
    // I/O multiplexing is driven by Server::run()
    if (fcntl(output_pipe[0], F_SETFL, O_NONBLOCK) == -1) {
        close(output_pipe[0]);
        freeEnv(env, env_vars.size());
        kill(pid, SIGKILL);
        waitpid(pid, NULL, 0);
        result.error       = true;
        result.status_code = 500;
        return result;
    }

    std::string raw_output;
    char        buf[CGI_BUF_SIZE];
    time_t      start = time(NULL);

    while (true) {
        // wall-clock timeout check
        if (time(NULL) - start > (time_t)timeout_sec) {
            kill(pid, SIGKILL);
            waitpid(pid, NULL, 0);
            close(output_pipe[0]);
            freeEnv(env, env_vars.size());
            result.timed_out   = true;
            result.status_code = 504;
            return result;
        }

        // non-blocking read — I/O multiplexing is in Server::run()
        ssize_t n = read(output_pipe[0], buf, sizeof(buf));
        if (n > 0) {
            raw_output.append(buf, static_cast<size_t>(n));
        } else if (n == 0) {
            break; // EOF
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            usleep(1000); // yield CPU, re-check timeout next iteration
            continue;
        } else {
            break; // real error
        }
    }

    close(output_pipe[0]);

    // reap the child — prevents zombie processes
    int child_status = 0;
    waitpid(pid, &child_status, 0);
    freeEnv(env, env_vars.size());

    // non-zero exit from the script → 500
    if (WIFEXITED(child_status) && WEXITSTATUS(child_status) != 0) {
        result.error       = true;
        result.status_code = 500;
        return result;
    }

    // parse CGI output into headers + body
    parseCgiOutput(raw_output, result.headers, result.body);
    result.status_code = 200;
    return result;
}