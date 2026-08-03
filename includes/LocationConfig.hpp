#pragma once

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
    const std::string &getCgiPath() const {return cgi_path_;}
    bool getAutoindex() const {return autoindex_;} 
    
private:
    std::string root_;                                  // Rootpath
    std::string path_;                                  // [path] : route
    std::vector<std::string> allow_methods_;            // GET POST DELETE etc.
    std::vector<std::string> index_;                    // Default html
    std::pair<uint16_t, std::string> redirection_;      // Redirection [status code] - [target url]
    std::string cgi_path_;                              // CGI pass [path]
    bool autoindex_;                                    // 1 : on, 0 : off
};