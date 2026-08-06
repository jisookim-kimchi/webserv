#include "../includes/ServerConfig.hpp"

ServerConfig::ServerConfig()
    : host_(""),
      server_name_(),
      port_(),
      error_page_path_(),
      client_max_body_size_(0),
      locations_() {
}

ServerConfig::~ServerConfig() {
}

ServerConfig::ServerConfig(const ServerConfig& other) {
    *this = other;
}

ServerConfig& ServerConfig::operator=(const ServerConfig& other) {
    if (this != &other) {
        host_ = other.host_;
        server_name_ = other.server_name_;
        port_ = other.port_;
        error_page_path_ = other.error_page_path_;
        client_max_body_size_ = other.client_max_body_size_;
        locations_ = other.locations_;
    }
    return *this;
}
