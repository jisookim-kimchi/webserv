#include "../includes/ConfigParser.hpp"
#include <cctype>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

namespace {

uint16_t parsePortStrict(const std::string& token) {
    if (token.empty())
        throw std::invalid_argument("bad port number: " + token);
    size_t idx = 0;
    unsigned long port = 0;
    try {
        port = std::stoul(token, &idx, 10);
    } catch (const std::exception&) {
        throw std::invalid_argument("bad port number: " + token);
    }
    if (idx != token.size() || port > 65535 || port == 0)
        throw std::invalid_argument("bad port number: " + token);
    return static_cast<uint16_t>(port);
}

uint64_t parseBodySizeStrict(const std::string& token) {
    if (token.empty() || !std::isdigit(static_cast<unsigned char>(token[0])))
        throw std::invalid_argument("bad client_max_body_size: " + token);

    size_t idx = 0;
    uint64_t value = 0;
    try {
        value = std::stoull(token, &idx, 10);
    } catch (const std::exception&) {
        throw std::invalid_argument("bad client_max_body_size: " + token);
    }

    if (idx == token.size())
        return value;
    if (idx + 1 != token.size())
        throw std::invalid_argument("bad client_max_body_size: " + token);

    const char unit = token[idx];
    if (unit == 'K' || unit == 'k')
        return value * 1024ULL;
    if (unit == 'M' || unit == 'm')
        return value * 1024ULL * 1024ULL;
    if (unit == 'G' || unit == 'g')
        return value * 1024ULL * 1024ULL * 1024ULL;
    throw std::invalid_argument("bad client_max_body_size: " + token);
}

}  // namespace

ConfigParser::ConfigParser() {
}
ConfigParser::~ConfigParser() {
}
ConfigParser::ConfigParser(const ConfigParser& other) {
    *this = other;
}
ConfigParser& ConfigParser::operator=(const ConfigParser& other) {
    if (this != &other) {
        server_configs_ = other.server_configs_;
    }
    return *this;
}

void ConfigParser::parse(const std::string& filename) {
    std::string file_data = readFile(filename);
    std::vector<std::string> tokens = tokenize(file_data);
    size_t index = 0;
    while (index < tokens.size()) {
        if (tokens[index] == "server") {
            parseServer(tokens, index);
        } else {
            throw std::invalid_argument("error : unexpected token outside server: " + tokens[index]);
        }
    }
}

/*
    @brief : read config file data
    @param1 : filename
    @return : string of file
    @throw : runtime_error if file is empty or not found
*/
std::string ConfigParser::readFile(const std::string& filename) {
    if (filename.empty())
        throw std::runtime_error("error : empty filename");

    std::ifstream file(filename.c_str());
    if (!file.is_open())
        throw std::runtime_error("error : open file : " + filename);

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/*
    @brief : split string into tokens
    @param1 : input of config file
    @return : vector of tokens
    @think : '/' case??
*/
std::vector<std::string> ConfigParser::tokenize(const std::string& input) {
    std::vector<std::string> tokens;
    std::string token;
    for (size_t i = 0; i < input.length(); i++) {
        if (input[i] == '#') {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            while (i < input.length() && input[i] != '\n')
                i++;
        } else if (isspace(input[i])) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else if (input[i] == '{' || input[i] == '}' || input[i] == ';') {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            tokens.push_back(std::string(1, input[i]));
        } else {
            token += input[i];
        }
    }
    if (!token.empty())
        tokens.push_back(token);
    return tokens;
}

/*
    @brief : parse server block
    @param1 : tokens
    @param2 : index of tokens
*/
void ConfigParser::parseServer(const std::vector<std::string>& tokens, size_t& index) {
    ServerConfig new_server;
    index++;
    if (index >= tokens.size() || tokens[index] != "{") {
        std::cout << "error : expected '{' after server\n";
        exit(1);
    }
    index++;
    while (index < tokens.size() && tokens[index] != "}") {
        if (tokens[index] == "location") {
            parseLocation(tokens, index, new_server);
        } else {
            parseServerKeyword(tokens, index, new_server);
        }
    }
    if (index >= tokens.size() || tokens[index] != "}") {
        std::cout << "error : expected '}' after server\n";
        exit(1);
    }
    index++;
    server_configs_.push_back(new_server);
}

/*
    @brief : parse location block
    @param1 : tokens
    @param2 : index of tokens
    @param3 : server config
*/
void ConfigParser::parseLocation(const std::vector<std::string>& tokens, size_t& index,
                                 ServerConfig& server) {
    LocationConfig new_location;
    index++;
    if (index >= tokens.size())
        throw std::runtime_error("error : expected path after location");
    new_location.setPath(tokens[index]);
    index++;
    if (index >= tokens.size() || tokens[index] != "{") {
        std::cout << "error : expected '{' after location path\n";
        exit(1);
    }
    index++;
    while (index < tokens.size() && tokens[index] != "}") {
        parseLocationKeyword(tokens, index, new_location);
    }
    if (index >= tokens.size() || tokens[index] != "}") {
        std::cout << "error : expected '}' after location\n";
        exit(1);
    }
    index++;
    server.addLocation(new_location);
}

/*
    @brief: parse server block keyword
    @param1: tokens
    @param2: index of tokens
    @param3: server config
*/
void ConfigParser::parseServerKeyword(const std::vector<std::string>& tokens, size_t& index,
                                      ServerConfig& server) {
    const std::string& found = tokens[index];
    if (found == "listen") {
        index++;
        if (index >= tokens.size() || tokens[index] == "}" || tokens[index] == ";") {
            throw std::invalid_argument("error: 'empty' port number for 'listen'");
        }
        server.addPort(parsePortStrict(tokens[index]));
        index++;
    } else if (found == "host") {
        index++;
        if (index >= tokens.size() || tokens[index] == "}" || tokens[index] == ";") {
            throw std::invalid_argument("error: 'empty' argument for 'host'");
        }
        server.setHost(tokens[index]);
        index++;
    } else if (found == "server_name") {
        index++;
        while (index < tokens.size() && tokens[index] != ";") {
            server.addServerName(tokens[index]);
            index++;
        }
    } else if (found == "client_max_body_size") {
        index++;
        if (index >= tokens.size() || tokens[index] == "}" || tokens[index] == ";") {
            throw std::invalid_argument("error: bad argument for 'client_max_body_size'");
        }
        server.setClientMaxBodySize(parseBodySizeStrict(tokens[index]));
        index++;
    } else if (found == "error_page") {
        index++;
        std::vector<std::string> args;

        while (index < tokens.size() && tokens[index] != ";") {
            args.push_back(tokens[index]);
            index++;
        }
        if (args.size() < 2) {
            throw std::invalid_argument("error: error_page requires status code and a file path");
        }
        std::string file_path = args.back();
        args.pop_back();
        for (size_t i = 0; i < args.size(); i++) {
            try {
                int code = std::stoi(args[i]);
                if (code < 100 || code > 599)
                    throw std::invalid_argument("status code out of range in error_page: " + args[i]);
                server.addErrorPagePath(code, file_path);
            } catch (const std::out_of_range&) {
                throw std::invalid_argument("status code out of range in error_page: " + args[i]);
            } catch (const std::invalid_argument&) {
                throw std::invalid_argument("bad status code in error_page: " + args[i]);
            }
        }
    } else {
        throw std::invalid_argument("unknown server keyword: " + tokens[index]);
    }

    if (index < tokens.size() && tokens[index] == ";")
        index++;
}

/*
    @brief: parse location block keyword
    @param1: tokens
    @param2: index of tokens
    @param3: location config
*/
void ConfigParser::parseLocationKeyword(const std::vector<std::string>& tokens, size_t& index,
                                        LocationConfig& location) {
    const std::string& found = tokens[index];
    if (found == "root") {
        index++;
        if (index < tokens.size() && tokens[index] != ";")
            location.setRoot(tokens[index]);
        index++;
    } else if (found == "index") {
        index++;
        while (index < tokens.size() && tokens[index] != ";") {
            location.addIndex(tokens[index]);
            index++;
        }
    } else if (found == "allow_methods") {
        index++;
        while (index < tokens.size() && tokens[index] != ";") {
            location.addAllowMethods(tokens[index]);
            index++;
        }
    } else if (found == "autoindex") {
        index++;
        if (index < tokens.size() && tokens[index] != ";") {
            if (tokens[index] == "on")
                location.setAutoindex(true);
            else if (tokens[index] == "off")
                location.setAutoindex(false);
        }
        index++;
    } else if (found == "return") {
        index++;
        if (index >= tokens.size() || tokens[index] == "}" || tokens[index] == ";") {
            throw std::runtime_error("error: missing arguments for 'return'");
        }
        uint16_t status_code = 0;
        std::string target_url = "";
        try {
            status_code = static_cast<uint16_t>(std::stoi(tokens[index]));
            if (status_code < 100 || status_code > 599)
                throw std::runtime_error("status code out of range in return: " + tokens[index]);
            index++;
        } catch (const std::out_of_range&) {
            throw std::runtime_error("status code out of range in return: " + tokens[index]);
        } 
        catch (const std::invalid_argument&) {
            throw std::runtime_error("invalid status code in return directive: " + tokens[index]);
        }
        if (index < tokens.size() && tokens[index] != ";") {
            target_url = tokens[index];
            index++;
        }
        location.setRedirection(status_code, target_url);
    } else if (found == "cgi_pass" || found == "cgi_path") {
        index++;
        if (index < tokens.size() && tokens[index] != ";")
            location.setCgiPass(tokens[index]);
        index++;
    } else if (found == "client_max_body_size")
    {
        index++;
        if (index >= tokens.size() || tokens[index] == "}" || tokens[index] == ";") {
            throw std::invalid_argument("error: bad argument for 'client_max_body_size'");
        }
        location.setClientMaxBodySize(parseBodySizeStrict(tokens[index]));
        index++;
    }
    else {
        throw std::runtime_error("unknown location keyword: " + tokens[index]);
    }

    if (index < tokens.size() && tokens[index] == ";")
        index++;
}
