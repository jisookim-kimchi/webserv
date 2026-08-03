#pragma once

#include <cstdint>
#include <string>   
#include <vector>
#include <map>

class LocationConfig;

class ServerConfig
{
public:
    ServerConfig();
    ~ServerConfig();
    ServerConfig &operator=(const ServerConfig& other);
    ServerConfig(const ServerConfig& other);

    /* GETTER */
    const std::string &getHost() const {return host_;} 
    const std::map<uint16_t, std::string> &getErrorPagePath() const {return error_page_path_;} 
    const std::vector<std::string> &getServerName() const {return server_name_;} 
    const std::vector<uint16_t> &getPort() const {return port_;} 
    const uint64_t getClientMaxBodySize() const {return client_max_body_size_;} 

private:
    std::string host_;                                  // Server IP Address
    std::vector<std::string>server_name_;               // Server Name
    std::vector<uint16_t> port_;                        // Port Number
    std::map<uint16_t, std::string> error_page_path_;   // Error Page Path
    uint64_t client_max_body_size_;                     // Max Body Size
    std::vector<LocationConfig> locations_;             // Location Block
};