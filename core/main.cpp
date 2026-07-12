#include "Server.hpp"

int main(int ac, char **av)
{
    if (ac != 2)
    {
        std::cerr << "Usage: " << av[0] << " <config_file>" << std::endl;
        return 1;
    }
    try {
        Server server;
        ConfigParser parser;
        std::vector<ServerConfig> configs = parser.parse(av[1]);
        signal(SIGPIPE, SIG_IGN);
        server.setup(configs);
        server.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}