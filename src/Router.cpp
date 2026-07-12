#include "Router.hpp"
#include <sys/stat.h>
#include <cstring>

Router::Router() {}
Router::Router(const Router& other) { (void)other; }
Router& Router::operator=(const Router& other) { (void)other; return *this; }
Router::~Router() {}

// ─── ROUTE MATCHING ───────────────────────────────────────────────────────────

const LocationConfig* Router::matchLocation(const std::string& uri,
                                             const ServerConfig& srv) const {
    const LocationConfig* best     = NULL;
    size_t                best_len = 0;

    for (size_t i = 0; i < srv.locations.size(); i++) {
        const LocationConfig& loc = srv.locations[i];


        if (uri.substr(0, loc.path.size()) == loc.path) {
    
            bool boundary = (uri.size() == loc.path.size())
                         || (uri[loc.path.size()] == '/')
                         || (loc.path == "/");

            if (boundary && loc.path.size() > best_len) {
                best     = &loc;
                best_len = loc.path.size();
            }
        }
    }
    return best;
}

// ─── PATH RESOLUTION ──────────────────────────────────────────────────────────

std::string Router::resolvePath(const std::string& uri,
                                 const LocationConfig& loc) const {
    std::string suffix;
    if (loc.path == "/")
        suffix = uri;
    else
        suffix = uri.substr(loc.path.size());

    if (!suffix.empty() && suffix[0] != '/')
        suffix = "/" + suffix;

    std::string root = loc.root;
    if (!root.empty() && root[root.size() - 1] == '/')
        root = root.substr(0, root.size() - 1);

    std::string full = root + suffix;

    if (!full.empty() && full[full.size() - 1] == '/' && !loc.index.empty())
        full += loc.index;

    return full;
}

// ─── EXTENSION ────────────────────────────────────────────────────────────────

std::string Router::getExtension(const std::string& path) const {
    size_t dot = path.rfind('.');
    if (dot == std::string::npos)
        return "";
    size_t slash = path.rfind('/');
    if (slash != std::string::npos && slash > dot)
        return "";
    return path.substr(dot);
}

// ─── CGI DETECTION ────────────────────────────────────────────────────────────

std::string Router::getCgiInterpreter(const std::string& path,
                                       const LocationConfig& loc) const {
    std::string ext = getExtension(path);
    if (ext.empty())
        return "";

    std::map<std::string, std::string>::const_iterator it =
        loc.cgi_pass.find(ext);

    if (it == loc.cgi_pass.end())
        return "";

    return it->second;
}