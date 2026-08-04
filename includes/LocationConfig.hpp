#pragma once

#include <stdint.h>
#include <string>
#include <vector>

class LocationConfig
{
public:
    LocationConfig();
    ~LocationConfig();
    LocationConfig(const LocationConfig& other);
    LocationConfig &operator=(const LocationConfig& other);

    /* GETTER */
    const std::string &getRoot() const {return root_;} 
    const std::string &getPath() const {return path_;} 
    const std::vector<std::string> &getAllowMethods() const {return allow_methods_;} 
    const std::vector<std::string> &getIndex() const {return index_;} 
    const std::pair<uint16_t, std::string> &getRedirection() const {return redirection_;} 
    const std::string &getCgiPass() const {return cgi_pass_;}
    const std::string &getCgiPath() const {return cgi_pass_;}
    bool getAutoindex() const {return autoindex_;} 
    
    /* SETTER */
    void setRoot(const std::string &root) {root_ = root;}
    void setPath(const std::string &path) {path_ = path;}
    void addAllowMethods(const std::string &allow_methods) {allow_methods_.push_back(allow_methods);}
    void addIndex(const std::string &index) {index_.push_back(index);}
    void setRedirection(const uint16_t &status_code, const std::string &target_url) {redirection_.first = status_code; redirection_.second = target_url;}
    void setCgiPass(const std::string &cgi_pass) {cgi_pass_ = cgi_pass;}
    void setCgiPath(const std::string &cgi_path) {cgi_pass_ = cgi_path;}
    void setAutoindex(bool autoindex) {autoindex_ = autoindex;}

private:
    std::string root_;                                  // Rootpath
    std::string path_;                                  // [path] : route
    std::vector<std::string> allow_methods_;            // GET POST DELETE etc.
    std::vector<std::string> index_;                    // Default html
    std::pair<uint16_t, std::string> redirection_;      // Redirection [status code] - [target url]
    std::string cgi_pass_;                              // CGI pass [path]
    bool autoindex_;                                    // 1 : on, 0 : off
};