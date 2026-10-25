#include "../includes/ConfigParser.hpp"
#include "../includes/Server.hpp"
#include <csignal>
#include <iostream>

int main(int argc, char** argv)
{
    std::signal(SIGPIPE, SIG_IGN);
    std::string configPath;
    if (argc == 2)
        configPath = argv[1];
    else if (argc == 1)
    {
        configPath = "./configs/Basic.config";
    }
    else
    {
        std::cerr << "usage : ./webserv [config file]\n";
        return 1;
    }
    try
    {
        ConfigParser parser;
        parser.parse(configPath);
        Server server(parser.getServerConfigs());
        server.run();
    } catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
