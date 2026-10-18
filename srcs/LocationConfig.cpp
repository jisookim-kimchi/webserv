#include "../includes/LocationConfig.hpp"

LocationConfig::LocationConfig()
    : root_(""),
      path_(""),
      allow_methods_(),
      index_(),
      redirection_(),
      cgi_pass_(""),
      autoindex_(false) {
}
