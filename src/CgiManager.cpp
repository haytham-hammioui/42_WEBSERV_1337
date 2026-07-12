#include "CgiManager.hpp"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define CGI_BUF_SIZE 4096

void CgiManager::add(int clientFd, pid_t pid, int pipeFd) {
    CgiProcess proc;
    proc.clientFd  = clientFd;
    proc.pid       = pid;
    proc.pipeFd    = pipeFd;
    proc.startTime = time(NULL);
    _procs[pipeFd] = proc;
}

ssize_t CgiManager::readOutput(int pipeFd) {
    std::map<int, CgiProcess>::iterator it = _procs.find(pipeFd);
    if (it == _procs.end())
        return -1;

    char buf[CGI_BUF_SIZE];
    ssize_t n = read(pipeFd, buf, sizeof(buf));
    if (n > 0)
        it->second.output.append(buf, static_cast<size_t>(n));
    return n;
}

std::string CgiManager::moveOutput(int pipeFd) {
    std::map<int, CgiProcess>::iterator it = _procs.find(pipeFd);
    if (it == _procs.end())
        return "";
    std::string out = it->second.output;
    it->second.output.clear();
    return out;
}

int CgiManager::getClientFd(int pipeFd) const {
    std::map<int, CgiProcess>::const_iterator it = _procs.find(pipeFd);
    if (it != _procs.end())
        return it->second.clientFd;
    return -1;
}

pid_t CgiManager::getPid(int pipeFd) const {
    std::map<int, CgiProcess>::const_iterator it = _procs.find(pipeFd);
    if (it != _procs.end())
        return it->second.pid;
    return -1;
}

bool CgiManager::hasProcess(int pipeFd) const {
    return _procs.find(pipeFd) != _procs.end();
}

std::vector<int> CgiManager::checkTimeouts(time_t now, int timeoutSec) {
    std::vector<int> timedOut;
    for (std::map<int, CgiProcess>::iterator it = _procs.begin();
         it != _procs.end(); ++it) {
        if (now - it->second.startTime > static_cast<time_t>(timeoutSec))
            timedOut.push_back(it->first);
    }
    return timedOut;
}

void CgiManager::remove(int pipeFd) {
    std::map<int, CgiProcess>::iterator it = _procs.find(pipeFd);
    if (it != _procs.end()) {
        close(it->second.pipeFd);
        _procs.erase(it);
    }
}

void CgiManager::killAll() {
    for (std::map<int, CgiProcess>::iterator it = _procs.begin();
         it != _procs.end(); ++it) {
        kill(it->second.pid, SIGKILL);
        waitpid(it->second.pid, NULL, WNOHANG);
        close(it->second.pipeFd);
    }
    _procs.clear();
}

CgiManager::~CgiManager() {
    killAll();
}
