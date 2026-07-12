#include "HttpHandlers.hpp"

#include <dirent.h>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

std::string readFile(std::string path)
{
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0)
        return "";

    std::string content;
    char buf[4096];
    ssize_t n;

    while ((n = read(fd, buf, sizeof(buf))) > 0)
        content.append(buf, n);

    close(fd);
    return content;
}

std::string getMimeType(std::string path)
{
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos)
        return "application/octet-stream";

    std::string ext = path.substr(dot + 1);

    if (ext == "html") return "text/html";
    if (ext == "css")  return "text/css";
    if (ext == "js")   return "application/javascript";
    if (ext == "png")  return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "pdf")  return "application/pdf";
    if (ext == "txt")  return "text/plain";

    return "application/octet-stream";
}

std::string buildListing(std::string path, std::string uri)
{
    DIR *dir = opendir(path.c_str());
    if (!dir)
        return "";

    std::ostringstream html;
    html << "<html><body><h1>Index of " << uri << "</h1><ul>";

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;
        if (name == ".")
            continue;
        html << "<li><a href=\"" << name << "\">" << name << "</a></li>";
    }
    closedir(dir);
    html << "</ul></body></html>";
    return html.str();
}

std::string handleStaticRequest(std::string uri, std::string root, std::string indexFile, bool autoindex, Req &req, Resp &resp)
{
    if (hasTraversal(uri))
        return sendError(resp, req, 400, "Bad Request");

    std::string path = getFilePath(uri, root);
    int type = checkPath(path);

    if (type == 404)
        return sendError(resp, req, 404, "Not Found");

    if (type == 1 && !uri.empty() && uri[uri.size() - 1] != '/')
    {
        resp.override_status(301)
            .set_headers("Location", uri + "/");
        return resp.build(req);
    }

    if (type == 1)
    {
        std::string indexPath = path;
        if (indexPath[indexPath.size() - 1] != '/')
            indexPath += "/";
        indexPath += indexFile;

        struct stat s;
        if (stat(indexPath.c_str(), &s) == 0 && S_ISREG(s.st_mode))
        {
            path = indexPath;
            type = 0;
        }
        else
        {
            if (autoindex)
            {
                resp.override_status(200)
                    .set_body(buildListing(path, uri))
                    .set_headers("Content-Type", "text/html");
            }
            else
                return sendError(resp, req, 403, "Forbidden");
            return resp.build(req);
        }
    }

    if (!canRead(path))
        return sendError(resp, req, 403, "Forbidden");

    std::string content = readFile(path);
    resp.override_status(200)
        .set_body(content)
        .set_headers("Content-Type", getMimeType(path));

    return resp.build(req);
}