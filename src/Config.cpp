#include "Config.hpp"

// LocationConfig
LocationConfig::LocationConfig()
        : path(""),
            methods(),
            root(""),
            index(""),
            directory_listing(false),
            upload_store(""),
            redirect(""),
            cgi_pass() {}

LocationConfig::LocationConfig(const LocationConfig& other) {
    *this = other;
}

LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
    if (this == &other)
        return *this;
    path              = other.path;
    methods           = other.methods;
    root              = other.root;
    index             = other.index;
    directory_listing = other.directory_listing;
    upload_store      = other.upload_store;
    redirect          = other.redirect;
    cgi_pass          = other.cgi_pass;
    return *this;
}

LocationConfig::~LocationConfig() {}

// ServerConfig
ServerConfig::ServerConfig()
    : port(80),
      host("0.0.0.0"),
      client_max_body_size(1048576) {}

ServerConfig::ServerConfig(const ServerConfig& other) {
    *this = other;
}

ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this == &other)
        return *this;
    port                 = other.port;
    host                 = other.host;
    client_max_body_size = other.client_max_body_size;
    error_pages          = other.error_pages;
    locations            = other.locations;
    return *this;
}

ServerConfig::~ServerConfig() {}