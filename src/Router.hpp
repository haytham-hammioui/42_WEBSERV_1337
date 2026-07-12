#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <string>
#include "Config.hpp"

class Router {
public:
    Router();
    Router(const Router& other);
    Router& operator=(const Router& other);
    ~Router();
    const LocationConfig* matchLocation(const std::string& uri,
                                        const ServerConfig& srv) const;
    std::string           resolvePath(const std::string& uri,
                                      const LocationConfig& loc) const;
    std::string           getExtension(const std::string& path) const;
    std::string           getCgiInterpreter(const std::string& path,
                                            const LocationConfig& loc) const;
};

#endif