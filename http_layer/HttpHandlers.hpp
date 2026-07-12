#pragma once

#include <string>

#include "Req.hpp"
#include "Resp.hpp"
#include "../src/Config.hpp"

std::string getFilePath(std::string uri, std::string root);
int         checkPath(std::string path);
bool        canRead(std::string path);
bool        canWrite(std::string path);
std::string getParentDir(const std::string &path);
bool        hasTraversal(const std::string &uri);

std::string toLowerCopy(std::string value);
std::string trimCopy(const std::string &value);
std::string extractBoundary(const std::string &contentType);
std::string extractDispositionParam(const std::string &value, const std::string &param);
bool        isSafeFilename(const std::string &filename);
std::string joinPath(const std::string &base, const std::string &name);
bool        writeFile(const std::string &path, const std::string &data);
bool        parseMultipartPart(const std::string &part, std::string &filename, std::string &data, bool &is_file);
bool        canWriteUploadRoot(const std::string &upload_dir);

std::string readFile(std::string path);
std::string getMimeType(std::string path);
std::string buildListing(std::string path, std::string uri);

std::string sendError(Resp &resp, const Req &req, int status, const std::string &reason,
                       const std::map<int, std::string> *error_pages = NULL);
std::string handleSessionRequest(Req &req, Resp &resp);

std::string handleStaticRequest(std::string uri, std::string root, std::string indexFile, bool autoindex, Req &req, Resp &resp);
std::string handleUploadRequest(std::string uri, std::string upload_dir, size_t max_body_size, Req &req, Resp &resp);
std::string handleDeleteRequest(std::string uri, std::string root, Req &req, Resp &resp);

std::string handleHttpRequest(const std::string &method,
                              const std::string &uri,
                              const LocationConfig &loc,
                              const ServerConfig &srv,
                              Req &req,
                              Resp &resp);