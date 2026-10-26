#pragma once

#include <stdint.h>
#include <string>
#include <vector>

class LocationConfig {
   public:
    LocationConfig();
    ~LocationConfig() = default;
    LocationConfig(const LocationConfig& other) = default;
    LocationConfig& operator=(const LocationConfig& other) = default;

    const std::string& getRoot() const {
        return root_;
    }
    const std::string& getPath() const {
        return path_;
    }
    const std::vector<std::string>& getAllowMethods() const {
        return allow_methods_;
    }
    const std::vector<std::string>& getIndex() const {
        return index_;
    }
    const std::pair<uint16_t, std::string>& getRedirection() const {
        return redirection_;
    }
    const std::string& getCgiPass() const {
        return cgi_pass_;
    }
    uint64_t getClientMaxBodySize() const {
        return client_max_body_size_;
    }

    bool hasClientMaxBodySize() const {
        return has_client_max_body_size_;
    }

    bool getAutoindex() const {
        return autoindex_;
    }

    void setRoot(const std::string& root) {
        root_ = root;
    }
    void setPath(const std::string& path) {
        path_ = path;
    }
    void addAllowMethods(const std::string& allow_methods) {
        allow_methods_.push_back(allow_methods);
    }
    void addIndex(const std::string& index) {
        index_.push_back(index);
    }
    void setRedirection(const uint16_t& status_code, const std::string& target_url) {
        redirection_.first = status_code;
        redirection_.second = target_url;
    }
    void setCgiPass(const std::string& cgi_pass) {
        cgi_pass_ = cgi_pass;
    }
    void setAutoindex(bool autoindex) {
        autoindex_ = autoindex;
    }
    void setClientMaxBodySize(const uint64_t& client_max_body_size) {
        client_max_body_size_ = client_max_body_size;
        has_client_max_body_size_ = true;
    }

   private:
    std::string root_;
    std::string path_;
    std::vector<std::string> allow_methods_;
    std::vector<std::string> index_;
    std::pair<uint16_t, std::string> redirection_;
    std::string cgi_pass_;
    uint64_t client_max_body_size_ = 0;
    bool has_client_max_body_size_ = false;
    bool autoindex_;
};
