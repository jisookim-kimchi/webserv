#pragma once

#include "ServerConfig.hpp"
#include <vector>
#include <string>

class ConfigParser
{
public:
    ConfigParser();
    ~ConfigParser();
    ConfigParser(const ConfigParser& other);
    ConfigParser &operator=(const ConfigParser& other);

    const std::vector<ServerConfig> &getServerConfigs() const { return server_configs_; }
    void parse(const std::string& filename);

private:
    std::string readFile(const std::string &filename);
    std::vector<std::string> tokenize(const std::string &line);
    
    void parseServer(const std::vector<std::string>& tokens, size_t &index);
    void parseLocation(const std::vector<std::string>& tokens, size_t &index, ServerConfig& server);

    void parseServerKeyword(const std::vector<std::string>& tokens, size_t& index, ServerConfig& server);
    void parseLocationKeyword(const std::vector<std::string>& tokens, size_t& index, LocationConfig& location);

    std::vector<ServerConfig> server_configs_;
};