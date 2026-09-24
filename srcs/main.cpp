#include "../includes/ConfigParser.hpp"
#include "../includes/Server.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    std::string configPath;
    if (argc == 2)
        configPath = argv[1];
    else
    {
        std::cerr << "usage : ./webserv [config file]\n";
        exit(1);
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
        exit(1);
    }

    return 0;
}
