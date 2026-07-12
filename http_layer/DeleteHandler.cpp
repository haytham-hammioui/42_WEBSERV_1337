#include "HttpHandlers.hpp"

#include <cerrno>
#include <unistd.h>

std::string handleDeleteRequest(std::string uri, std::string root, Req &req, Resp &resp)
{
    std::string path = getFilePath(uri, root);
    int type = checkPath(path);

    if (hasTraversal(uri))
        return sendError(resp, req, 400, "Bad Request");

    if (type == 404)
        return sendError(resp, req, 404, "Not Found");

    if (type == 1)
        return sendError(resp, req, 403, "Forbidden");

    std::string parent = getParentDir(path);
    if (parent.empty() || !canWrite(parent) || !canWrite(path))
        return sendError(resp, req, 403, "Forbidden");

    if (unlink(path.c_str()) != 0)
    {
        if (errno == EACCES || errno == EPERM)
            return sendError(resp, req, 403, "Forbidden");
        return sendError(resp, req, 500, "Internal Server Error");
    }

    resp.override_status(204);
    return resp.build(req);
}
