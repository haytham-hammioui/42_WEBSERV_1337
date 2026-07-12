#include "Client.hpp"

Client::Client()
{
}

Client::Client(int socketFd)
{
    fd = socketFd;
    state =READING;

    lastActivity = time(NULL);

    bytesSent = 0;
}

Client& Client::operator=(const Client& other) {
        if (this != &other) {
            fd = other.fd;
            state = other.state;
            writeBuffer = other.writeBuffer;
            lastActivity = other.lastActivity;
            parser = other.parser;
            bytesSent = other.bytesSent;
            serverConfig = other.serverConfig;
        }
        return *this;
    }