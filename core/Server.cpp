#include "Server.hpp"

Server::Server() {}

void Server::setup(const std::vector<ServerConfig>& configs)
{
    for (size_t i = 0; i < configs.size(); i++)
        createListeningSocket(configs[i]);
}


// ---------------- SOCKET ----------------

void Server::createListeningSocket(const ServerConfig& config)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));

    if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
    {
        perror("fcntl");
        close(fd);
        return;
    }

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(config.port);
    if (inet_pton(AF_INET,
              config.host.c_str(),
              &addr.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(fd);
        return;
    }

    if (bind(fd, (sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        close(fd);
        return;
    }

    if (listen(fd, 10) == -1)
    {
        perror("listen");
        close(fd);
        return;
    }

    pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    _fds.push_back(pfd);
    _Servers[fd] = config;
    _listeningSockets.insert(fd);

    std::cout << "Listening on port " << config.port << std::endl;
}

// ---------------- MAIN LOOP ----------------

void Server::run()
{
    while (true)
    {
        if (_fds.empty())
        {
            usleep(100000);
            continue;
        }
        for (size_t i = 0; i < _fds.size(); i++)
            _fds[i].revents = 0;
        int ret = poll(&_fds[0], _fds.size(), 1000); // 1000 milliseconds = 1 second is the timeout for the poll function. If no events occur within this time, poll will return 0, allowing the server to perform other tasks or checks (like timeouts) before polling again.
        if (ret == -1)
        {
            perror("poll");
            continue;
        }

        for (size_t i = 0; i < _fds.size(); ++i)
        {
            int fd = _fds[i].fd;

            // ---------------- ERROR ----------------
            if (_fds[i].revents & (POLLHUP | POLLERR))
            {
                if (_listeningSockets.count(fd))
                {
                    std::cerr << "Listening socket error!\n";
                    continue;
                }

                removeClient(i);
                --i;
                continue;
            }
            
            // ---------------- READ ----------------
            if (_fds[i].revents & POLLIN)
            {
                if (_listeningSockets.count(fd))
                {
                    acceptClient(fd);
                    continue;
                }

                handleRead(i);

                if (i >= _fds.size())  // safety after erase
                    break;

                fd = _fds[i].fd;
            }

            // ---------------- WRITE ----------------
            if (_fds[i].revents & POLLOUT)
            {
                handleWrite(i);
                if (i >= _fds.size()) break; // safety after erase
            }

            // ---------------- CLOSE STATE ----------------
            if (!_listeningSockets.count(fd))
            {
                Client &client = _clients[fd];

                if (client.state == CLOSING)
                {
                    removeClient(i);
                    --i;
                    continue;
                }
            }
        }

        checkTimeouts(); // Check for client timeouts after processing all events. This ensures that even if a client is idle and doesn't trigger any events, it will still be checked for timeouts and removed if necessary.
    }
}

// ---------------- ACCEPT ----------------

void Server::acceptClient(int listeningFd)
{
    int clientFd = accept(listeningFd, NULL, NULL);

    if (clientFd == -1)
    {
        perror("accept");
        return;
    }

    if (fcntl(clientFd, F_SETFL, O_NONBLOCK) == -1)
    {
        perror("fcntl");
        close(clientFd);
        return;
    }

    pollfd pfd;
    pfd.fd = clientFd;
    pfd.events = POLLIN;
    pfd.revents = 0;

    _fds.push_back(pfd);
    Client client(clientFd);
    client.serverConfig = _Servers[listeningFd];
    _clients[clientFd] = client;

    std::cout << "Client connected on port "
              << _Servers[listeningFd].port << std::endl;
}

// ---------------- READ ----------------

void Server::handleRead(size_t i)
{
    char buffer[BUFFER_SIZE];
    int fd = _fds[i].fd;

    Client &client = _clients[fd];
    if (client.state == CLOSING)
        return;

    ssize_t bytes = read(fd, buffer, sizeof(buffer));

    if (bytes == 0)
    {
        removeClient(i);
        return;
    }

    if (bytes < 0)
    {
        perror("read");
        removeClient(i);
        return;
    }
    client.parser.feed(std::string(buffer, bytes));

    client.lastActivity = time(NULL);

    if (client.parser.isComplete() == 3 || client.parser.get_status_code() >= 400)
    {
        processRequest(client);

        _fds[i].events &= ~POLLIN;
        _fds[i].events |= POLLOUT;
    }
}

// ---------------- PROCESS ----------------

void Server::processRequest(Client &client)
{
    Resp response;

    int status = client.parser.get_status_code();
    if (status >= 400)
    {
        client.writeBuffer = sendError(response,
                                       client.parser,
                                       status,
                                       "",
                                       &client.serverConfig.error_pages);
        client.bytesSent = 0;
        client.state = WRITING;
        return;
    }

    std::string uri = client.parser.getUri();
    size_t qpos = uri.find('?');
    if (qpos != std::string::npos)
        uri = uri.substr(0, qpos);

    Router router;

    const LocationConfig *loc =
        router.matchLocation(uri, client.serverConfig);

    if (!loc)
    {
        client.writeBuffer = sendError(response,
                                       client.parser,
                                       404,
                                       "Not Found",
                                       &client.serverConfig.error_pages);
    }
    else{
        client.writeBuffer =
            handleHttpRequest(
                client.parser.getMethod(),
                uri, *loc,
                client.serverConfig,
                client.parser,
                response);
    }

    client.bytesSent = 0;

    client.state = WRITING;
}

// ---------------- WRITE ----------------

void Server::handleWrite(size_t i)
{
    int fd = _fds[i].fd;
    Client &client = _clients[fd];

    if (client.writeBuffer.empty())
    {
        _fds[i].events &= ~POLLOUT;
        client.state = CLOSING;
        return;
    }

    ssize_t bytes = send(
        fd,
        client.writeBuffer.c_str() + client.bytesSent,
        client.writeBuffer.size() - client.bytesSent,
        0);
    

    if (bytes > 0)
    {
        client.bytesSent += bytes;
        client.lastActivity = time(NULL);

        if (client.bytesSent == client.writeBuffer.size())
        {
            client.writeBuffer.clear();
            client.bytesSent = 0;

            _fds[i].events &= ~POLLOUT;

            std::string conn = client.parser.get_header("connection");
            if (conn == "close")
            {
                client.state = CLOSING;
            }
            else
            {
                client.parser.clear();
                client.state = READING;
                _fds[i].events |= POLLIN;
            }
        }

        return;
    }

    if (bytes < 0)
    {
        perror("send");
        removeClient(i);
        return;
    }

    if (bytes == 0)
    {
        client.state = CLOSING;
        return;
    }
}

// ---------------- TIMEOUT ----------------

void Server::checkTimeouts() // Check for clients that have been idle for too long and remove them from the server. This helps to free up resources and prevent the server from being overwhelmed by inactive clients.
{
    time_t now = time(NULL);

    for (size_t i = 0; i < _fds.size(); ++i)
    {
        int fd = _fds[i].fd;

        if (_listeningSockets.count(fd))
            continue;

        Client &client = _clients[fd];

        if (now - client.lastActivity > TIMEOUT) // If the client has been idle for more than 60 seconds, consider it timed out and remove it from the server.
        {
            std::cout << "Client timed out\n";
            removeClient(i);
            --i;
        }
    }
}

// ---------------- REMOVE ----------------

void Server::removeClient(size_t i)
{
    int fd = _fds[i].fd;
    std::cout << "Client disconnected\n";

    close(fd);
    _clients.erase(fd);
    _fds.erase(_fds.begin() + i);
}

// ---------------- DESTRUCTOR ----------------

Server::~Server()
{
    for (size_t i = 0; i < _fds.size(); i++)
        close(_fds[i].fd);

    _clients.clear();
    _Servers.clear();
    _listeningSockets.clear();
}
       