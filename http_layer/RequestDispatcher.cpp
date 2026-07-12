#include "HttpHandlers.hpp"
#include "../src/Router.hpp"
#include "../src/CgiHandler.hpp"
#include <sstream>
#include <sys/stat.h>

static bool methodAllowed(const std::string &method,
                          const LocationConfig &loc)
{
    if (method == "HEAD")
        return true;
    for (size_t i = 0; i < loc.methods.size(); i++)
        if (loc.methods[i] == method)
            return true;
    return false;
}

static std::string stripLocation(const std::string &uri,
                                  const LocationConfig &loc)
{
    if (loc.path != "/" && uri.substr(0, loc.path.size()) == loc.path)
        return uri.substr(loc.path.size());
    return uri;
}

std::string handleHttpRequest(const std::string &method,
                              const std::string &uri,
                              const LocationConfig &loc,
                              const ServerConfig &srv,
                              Req &req,
                              Resp &resp)
{
    if (!loc.redirect.empty())
    {
        resp.override_status(302)
            .set_headers("Location", loc.redirect);
        return resp.build(req);
    }

    if (!methodAllowed(method, loc))
        return sendError(resp, req, 405, "Method Not Allowed",
                         &srv.error_pages);

    std::string body = req.get_body();
    if (body.size() > srv.client_max_body_size)
        return sendError(resp, req, 413, "Payload Too Large",
                         &srv.error_pages);

    // ── CGI check ─────────────────────────────────────────────────────
    {
        Router router;
        std::string script_path = router.resolvePath(uri, loc);
        std::string interpreter = router.getCgiInterpreter(script_path, loc);

        if (!interpreter.empty())
        {
            struct stat st;
            if (stat(script_path.c_str(), &st) != 0 || !S_ISREG(st.st_mode))
                return sendError(resp, req, 404, "Not Found",
                                 &srv.error_pages);

            std::map<std::string, std::string> env;
            env["REQUEST_METHOD"]     = method;
            env["SCRIPT_FILENAME"]    = script_path;
            env["SCRIPT_NAME"]        = uri;
            env["SERVER_PROTOCOL"]    = "HTTP/1.1";
            env["GATEWAY_INTERFACE"]  = "CGI/1.1";
            env["SERVER_SOFTWARE"]    = "webserv/1.0";
            env["SERVER_NAME"]        = srv.host;
            {
                std::ostringstream ss;
                ss << srv.port;
                env["SERVER_PORT"] = ss.str();
            }

            {
                size_t qpos = req.getUri().find('?');
                if (qpos != std::string::npos)
                    env["QUERY_STRING"] = req.getUri().substr(qpos + 1);
            }

            {
                std::string ct = req.get_header("content-type");
                if (!ct.empty())
                    env["CONTENT_TYPE"] = ct;
            }

            {
                std::ostringstream ss;
                ss << body.size();
                env["CONTENT_LENGTH"] = ss.str();
            }

            CgiHandler cgi;
            CgiResult result = cgi.execute(interpreter, script_path,
                                            body, env, 10);

            if (result.timed_out)
                return sendError(resp, req, 504, "Gateway Timeout",
                                 &srv.error_pages);

            if (result.error)
                return sendError(resp, req, 500, "Internal Server Error",
                                 &srv.error_pages);

            std::istringstream hstream(result.headers);
            std::string line;
            bool has_status = false;
            while (std::getline(hstream, line))
            {
                if (!line.empty() && line[line.size() - 1] == '\r')
                    line.erase(line.size() - 1);
                size_t colon = line.find(':');
                if (colon == std::string::npos)
                    continue;

                std::string key = line.substr(0, colon);
                std::string value = line.substr(colon + 1);
                size_t start = value.find_first_not_of(" \t");
                if (start != std::string::npos)
                    value = value.substr(start);

                if (key == "Status")
                {
                    has_status = true;
                    std::istringstream ss(value);
                    int status;
                    ss >> status;
                    if (status > 0)
                        resp.override_status(status);
                    continue;
                }

                resp.set_headers(key, value);
            }

            if (!has_status)
                resp.override_status(result.status_code);

            resp.set_body(result.body);
            std::string raw = resp.build(req);

            if (method == "HEAD")
            {
                size_t sep = raw.find("\r\n\r\n");
                if (sep != std::string::npos)
                    raw = raw.substr(0, sep + 4);
            }

            return raw;
        }
    }

    if (method == "GET" || method == "HEAD")
    {
        // Session handler on root path
        if (uri == "/" || uri.empty())
        {
            std::string raw = handleSessionRequest(req, resp);
            if (method == "HEAD")
            {
                size_t sep = raw.find("\r\n\r\n");
                if (sep != std::string::npos)
                    raw = raw.substr(0, sep + 4);
            }
            return raw;
        }

        std::string adj = stripLocation(uri, loc);
        std::string raw = handleStaticRequest(adj, loc.root, loc.index,
                                                loc.directory_listing, req, resp);
        if (method == "HEAD")
        {
            size_t sep = raw.find("\r\n\r\n");
            if (sep != std::string::npos)
                raw = raw.substr(0, sep + 4);
        }
        return raw;
    }

    if (method == "POST")
    {
        if (loc.upload_store.empty())
            return sendError(resp, req, 403, "Forbidden",
                             &srv.error_pages);

        std::string ct = toLowerCopy(req.get_header("content-type"));
        if (ct.find("multipart/form-data") != std::string::npos)
            return handleUploadRequest(uri, loc.upload_store, srv.client_max_body_size, req, resp);

        if (body.empty())
            return sendError(resp, req, 400, "Bad Request",
                             &srv.error_pages);

        std::string filename = uri;
        size_t slash = filename.rfind('/');
        if (slash != std::string::npos)
            filename = filename.substr(slash + 1);
        if (filename.empty())
            filename = "upload";

        if (!isSafeFilename(filename))
            return sendError(resp, req, 400, "Bad Request",
                             &srv.error_pages);

        std::string path = joinPath(loc.upload_store, filename);
        if (!writeFile(path, body))
            return sendError(resp, req, 500, "Internal Server Error",
                             &srv.error_pages);

        return sendError(resp, req, 201, "Created",
                         &srv.error_pages);
    }

    if (method == "DELETE")
    {
        std::string adj = stripLocation(uri, loc);
        return handleDeleteRequest(adj, loc.root, req, resp);
    }

    return sendError(resp, req, 405, "Method Not Allowed",
                     &srv.error_pages);
}
