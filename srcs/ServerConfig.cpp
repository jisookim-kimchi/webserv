#include "../includes/ServerConfig.hpp"

ServerConfig::ServerConfig()
    : host_(""),
      server_name_(),
      port_(),
      error_page_path_(),
      client_max_body_size_(0),
      locations_() {
}
