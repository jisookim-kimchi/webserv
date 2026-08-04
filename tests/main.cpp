#include "../includes/ConfigParser.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "error : need to input config file" << std::endl;
        return 1;
    }
    ConfigParser parser;
    parser.parse(argv[1]);

    const std::vector<ServerConfig>& servers = parser.getServerConfigs();
    std::ofstream outfile("tests/parseServer_parseLocation.txt");

    for (size_t i = 0; i < servers.size(); i++)
    {
        outfile << "host    " << servers[i].getHost() << std::endl;
        
        const std::vector<uint16_t>& ports = servers[i].getPort();
        for (size_t j = 0; j < ports.size(); j++)
        {
            outfile << "listen    " << ports[j] << std::endl;
        }
        const std::vector<std::string>& names = servers[i].getServerName();
        for (size_t j = 0; j < names.size(); j++)
        {
            outfile << "server_name     " << names[j] << std::endl;
        }
        outfile << "client_max_body_size    " << servers[i].getClientMaxBodySize() << std::endl;
        const std::map<uint16_t, std::string>& errs = servers[i].getErrorPagePath();
        for (auto& [key, value] : errs)
        {
            outfile << "error_page     " << key << " " << value << std::endl;
        }
    }
    
    outfile.close();


    return 0;
}