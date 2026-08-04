#pragma once

#include <stdint.h>
#include <string>   
#include <vector>
#include <map>

#include "LocationConfig.hpp"

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
    uint64_t getClientMaxBodySize() const {return client_max_body_size_;} 
    const std::vector<LocationConfig>& getLocations() const {return locations_;}

    /* SETTER */
    void setHost(const std::string &host) {host_ = host;}
    void setClientMaxBodySize(uint64_t client_max_body_size) {client_max_body_size_ = client_max_body_size;}
    void addErrorPagePath(const uint16_t &error_code, const std::string &file_path) {error_page_path_[error_code] = file_path;}
    void addServerName(const std::string &server_name) {server_name_.push_back(server_name);}
    void addPort(const uint16_t port) {port_.push_back(port);}
    void addLocation(const LocationConfig &location) {locations_.push_back(location);}
    
private:
    std::string host_;                                  // Server IP Address
    std::vector<std::string>server_name_;               // Server Name
    std::vector<uint16_t> port_;                        // Port Number
    std::map<uint16_t, std::string> error_page_path_;   // Error Page Path
    uint64_t client_max_body_size_;                     // Max Body Size
    std::vector<LocationConfig> locations_;             // Location Block
};