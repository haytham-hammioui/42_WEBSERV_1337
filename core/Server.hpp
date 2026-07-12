#ifndef SERVER_HPP
#define SERVER_HPP

#include <vector>
#include <map>
#include <poll.h>
#include "Client.hpp"
#include <iostream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <set>
#include <signal.h>
#include <sstream>
#include "../http_layer/Req.hpp"
#include "../http_layer/Resp.hpp"
#include "../http_layer/HttpHandlers.hpp"
#include "../src/Config.hpp"
#include "../src/ConfigParser.hpp"
#include "../src/Router.hpp"

#define BUFFER_SIZE 16384
#define TIMEOUT 30

class Server
{
    private:
        std::map<int, ServerConfig> _Servers; //Many listening sockets.fd -> serverConfig(port, host, etc.)
        std::set<int> _listeningSockets;
        std::vector<pollfd> _fds;
        std::map<int, Client> _clients;

    public:

        Server();
        void setup(const std::vector<ServerConfig>& configs);
        void run();
        void acceptClient(int listeningFd);
        void handleRead(size_t i);
        void handleWrite(size_t i);
        void removeClient(size_t i);
        void checkTimeouts();
        void createListeningSocket(const ServerConfig& config);
        void processRequest(Client &client);
        ~Server();
};



#endif