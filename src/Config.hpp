#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <string>
#include <vector>
#include <map>

struct LocationConfig {
    std::string                        path;
    std::vector<std::string>           methods;
    std::string                        root;
    std::string                        index;
    bool                               directory_listing;
    std::string                        upload_store;
    std::string                        redirect;
    std::map<std::string, std::string> cgi_pass;

    LocationConfig();
    LocationConfig(const LocationConfig& other);
    LocationConfig& operator=(const LocationConfig& other);
    ~LocationConfig();
};

struct ServerConfig {
    int                                port;
    std::string                        host;
    size_t                             client_max_body_size;
    std::map<int, std::string>         error_pages;
    std::vector<LocationConfig>        locations;

    ServerConfig();
    ServerConfig(const ServerConfig& other);
    ServerConfig& operator=(const ServerConfig& other);
    ~ServerConfig();
};

#endif