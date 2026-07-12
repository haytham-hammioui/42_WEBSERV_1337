#include "HttpHandlers.hpp"
#include "SessionStore.hpp"

#include <cctype>
#include <cstdlib>
#include <cerrno>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

std::string getFilePath(std::string uri, std::string root)
{
    return root + uri;
}

int checkPath(std::string path)
{
    struct stat s;

    if (stat(path.c_str(), &s) != 0)
        return 404;

    if (S_ISDIR(s.st_mode))
        return 1;

    if (S_ISREG(s.st_mode))
        return 0;

    return 404;
}

bool canRead(std::string path)
{
    return access(path.c_str(), R_OK) == 0;
}

bool canWrite(std::string path)
{
    return access(path.c_str(), W_OK) == 0;
}

std::string getParentDir(const std::string &path)
{
    size_t slash = path.find_last_of('/');
    if (slash == std::string::npos)
        return "";
    if (slash == 0)
        return "/";
    return path.substr(0, slash);
}

bool hasTraversal(const std::string &uri)
{
    return uri.find("..") != std::string::npos;
}

std::string toLowerCopy(std::string value)
{
    for (size_t i = 0; i < value.size(); i++)
        value[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(value[i])));
    return value;
}

std::string trimCopy(const std::string &value)
{
    size_t begin = value.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
        return "";
    size_t end = value.find_last_not_of(" \t\r\n");
    return value.substr(begin, end - begin + 1);
}

std::string extractBoundary(const std::string &contentType)
{
    std::string lowered = toLowerCopy(contentType);
    if (lowered.find("multipart/form-data") == std::string::npos)
        return "";

    size_t pos = lowered.find("boundary=");
    if (pos == std::string::npos)
        return "";

    pos += 9;
    if (pos >= contentType.size())
        return "";

    std::string boundary = contentType.substr(pos);
    boundary = trimCopy(boundary);
    if (boundary.empty())
        return "";

    if (boundary[0] == '"')
    {
        size_t end = boundary.find('"', 1);
        if (end == std::string::npos)
            return "";
        boundary = boundary.substr(1, end - 1);
    }
    else
    {
        size_t end = boundary.find_first_of("; \t\r\n");
        if (end != std::string::npos)
            boundary = boundary.substr(0, end);
    }

    boundary = trimCopy(boundary);
    if (boundary.empty())
        return "";
    return boundary;
}

std::string extractDispositionParam(const std::string &value, const std::string &param)
{
    std::string lowered = toLowerCopy(value);
    std::string needle = toLowerCopy(param) + "=";
    size_t pos = lowered.find(needle);
    if (pos == std::string::npos)
        return "";

    pos += needle.size();
    while (pos < value.size() && value[pos] == ' ')
        pos++;
    if (pos >= value.size())
        return "";

    if (value[pos] == '"')
    {
        size_t end = value.find('"', pos + 1);
        if (end == std::string::npos)
            return "";
        return value.substr(pos + 1, end - pos - 1);
    }

    size_t end = value.find(';', pos);
    if (end == std::string::npos)
        end = value.size();
    return trimCopy(value.substr(pos, end - pos));
}

bool isSafeFilename(const std::string &filename)
{
    if (filename.empty())
        return false;
    if (filename.find('/') != std::string::npos)
        return false;
    if (filename.find('\\') != std::string::npos)
        return false;
    if (filename.find("..") != std::string::npos)
        return false;
    return true;
}

std::string joinPath(const std::string &base, const std::string &name)
{
    if (base.empty())
        return name;
    if (base[base.size() - 1] == '/')
        return base + name;
    return base + "/" + name;
}

bool writeFile(const std::string &path, const std::string &data)
{
    int fd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0)
        return false;

    size_t written = 0;
    while (written < data.size())
    {
        ssize_t n = write(fd, data.c_str() + written, data.size() - written);
        if (n < 0)
        {
            close(fd);
            unlink(path.c_str());
            return false;
        }
        written += static_cast<size_t>(n);
    }

    close(fd);
    return true;
}

bool parseMultipartPart(const std::string &part, std::string &filename, std::string &data, bool &is_file)
{
    size_t split = part.find("\r\n\r\n");
    if (split == std::string::npos)
        return false;

    std::string headers = part.substr(0, split);
    data = part.substr(split + 4);
    is_file = false;
    filename.clear();

    std::istringstream lines(headers);
    std::string line;
    bool has_disposition = false;

    while (std::getline(lines, line))
    {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        size_t colon = line.find(':');
        if (colon == std::string::npos)
            return false;

        std::string key = toLowerCopy(trimCopy(line.substr(0, colon)));
        std::string value = trimCopy(line.substr(colon + 1));

        if (key == "content-disposition")
        {
            has_disposition = true;
            std::string lowered = toLowerCopy(value);
            if (lowered.find("form-data") == std::string::npos)
                return false;

            filename = extractDispositionParam(value, "filename");
            if (!filename.empty())
                is_file = true;
        }
    }

    if (!has_disposition)
        return false;
    return true;
}

std::string sendError(Resp &resp, const Req &req, int status, const std::string &reason,
                       const std::map<int, std::string> *error_pages)
{
    if (error_pages)
    {
        std::map<int, std::string>::const_iterator it = error_pages->find(status);
        if (it != error_pages->end())
        {
            std::string body = readFile(it->second);
            if (!body.empty())
            {
                resp.override_status(status)
                    .set_body(body)
                    .set_headers("Content-Type", "text/html")
                    .set_headers("Connection", "close");
                return resp.build(req);
            }
        }
    }
    std::string reason_text = reason;
    if (reason_text.empty())
        reason_text = Resp::reason_phrase(status);
    {
        std::ostringstream ss;
        ss << status;
        resp.override_status(status)
            .set_body("<html><body><h1>" + ss.str() + " " + reason_text + "</h1></body></html>")
            .set_headers("Content-Type", "text/html")
            .set_headers("Connection", "close");
    }
    return resp.build(req);
}

bool canWriteUploadRoot(const std::string &upload_dir)
{
    struct stat s;
    if (stat(upload_dir.c_str(), &s) != 0)
        return false;
    if (!S_ISDIR(s.st_mode))
        return false;
    return access(upload_dir.c_str(), W_OK) == 0;
}

std::string handleSessionRequest(Req &req, Resp &resp)
{
    SessionStore &store = SessionStore::instance();
    std::string sid = req.get_cookie("session_id");

    Session *session = NULL;
    if (!sid.empty())
        session = store.get(sid);

    if (!session)
    {
        session = store.create();
        resp.set_cookie("session_id", session->id, 3600, "/", true, false);
        session->data["visits"] = "0";
    }
    else
    {
        int visits = 1;
        if (!session->data["visits"].empty())
            visits = std::atoi(session->data["visits"].c_str()) + 1;
        std::ostringstream oss;
        oss << visits;
        session->data["visits"] = oss.str();
        resp.set_body("<html><body><h1>Welcome back</h1><p>Session ID: "
                      + session->id + "</p><p>Visits: "
                      + session->data["visits"] + "</p></body></html>");
    }

    resp.set_headers("Content-Type", "text/html");
    resp.override_status(200);
    return resp.build(req);
}