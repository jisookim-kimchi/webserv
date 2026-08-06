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

LocationConfig::~LocationConfig() {
}

LocationConfig::LocationConfig(const LocationConfig& other) {
    *this = other;
}

LocationConfig& LocationConfig::operator=(const LocationConfig& other) {
    if (this != &other) {
        root_ = other.root_;
        path_ = other.path_;
        allow_methods_ = other.allow_methods_;
        index_ = other.index_;
        redirection_ = other.redirection_;
        cgi_pass_ = other.cgi_pass_;
        autoindex_ = other.autoindex_;
    }
    return *this;
}
