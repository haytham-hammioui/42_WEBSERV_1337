#include "HttpHandlers.hpp"

std::string handleUploadRequest(std::string uri, std::string upload_dir, size_t max_body_size, Req &req, Resp &resp)
{
    (void)uri;

    if (upload_dir.empty() || !canWriteUploadRoot(upload_dir))
        return sendError(resp, req, 403, "Forbidden");

    std::string body = req.get_body();
    if (body.size() > max_body_size)
        return sendError(resp, req, 413, "Payload Too Large");

    std::string content_type = req.get_header("content-type");
    std::string boundary = extractBoundary(content_type);
    if (content_type.empty() || toLowerCopy(content_type).find("multipart/form-data") == std::string::npos)
        return sendError(resp, req, 415, "Unsupported Media Type");
    if (boundary.empty())
        return sendError(resp, req, 400, "Bad Request");

    std::string delimiter = "--" + boundary;
    if (body.compare(0, delimiter.size(), delimiter) != 0)
        return sendError(resp, req, 400, "Bad Request");

    size_t cursor = delimiter.size();
    if (body.compare(cursor, 2, "--") == 0)
        return sendError(resp, req, 400, "Bad Request");
    if (body.compare(cursor, 2, "\r\n") != 0)
        return sendError(resp, req, 400, "Bad Request");
    cursor += 2;

    bool wrote_file = false;
    std::string boundary_marker = std::string("\r\n") + delimiter;

    while (true)
    {
        size_t next = body.find(boundary_marker, cursor);
        if (next == std::string::npos)
            return sendError(resp, req, 400, "Bad Request");

        std::string part = body.substr(cursor, next - cursor);
        std::string filename;
        std::string data;
        bool is_file = false;

        if (!parseMultipartPart(part, filename, data, is_file))
            return sendError(resp, req, 400, "Bad Request");

        if (is_file)
        {
            if (!isSafeFilename(filename))
                return sendError(resp, req, 400, "Bad Request");

            std::string path = joinPath(upload_dir, filename);
            if (!writeFile(path, data))
                return sendError(resp, req, 500, "Internal Server Error");
            wrote_file = true;
        }

        cursor = next + 2 + delimiter.size();
        if (body.compare(cursor, 2, "--") == 0)
            break;
        if (body.compare(cursor, 2, "\r\n") == 0)
            cursor += 2;
        else
            return sendError(resp, req, 400, "Bad Request");
    }

    if (!wrote_file)
        return sendError(resp, req, 400, "Bad Request");

    return sendError(resp, req, 201, "Created");
}
