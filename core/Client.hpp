#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <ctime>
#include "../http_layer/Req.hpp"
#include "../src/Config.hpp"

enum ClientState{
    READING,
    PROCESSING,
    WRITING,
    CLOSING
};

class Client
{
public:
    int fd;
    ClientState state;
    ServerConfig serverConfig;

    std::string writeBuffer;

    time_t lastActivity;

    Req parser;
    size_t bytesSent;

    Client();
    Client(int fd);
    Client& operator=(const Client& other);
};
#endif