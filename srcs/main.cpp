#include "../includes/ConfigParser.hpp"
#include "../includes/Server.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    std::string configPath = "configs/test.config";
    if (argc == 2)
        configPath = argv[1];
    else if (argc > 2)
    {
        std::cerr << "too many arguments\n";
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
