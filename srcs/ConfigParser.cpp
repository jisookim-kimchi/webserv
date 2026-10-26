#include "../includes/ConfigParser.hpp"
#include <cctype>
#include <charconv>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <fstream>
#include <sstream>

namespace {

uint16_t parsePortStrict(const std::string& token) {
    if (token.empty())
        throw std::invalid_argument("error: bad port number: " + token);
    unsigned long port = 0;
    const auto result =
        std::from_chars(token.data(), token.data() + token.size(), port, 10);
    if (result.ec != std::errc() || result.ptr != token.data() + token.size() || port == 0 ||
        port > 65535)
        throw std::invalid_argument("error: bad port number: " + token);
    return static_cast<uint16_t>(port);
}

uint64_t parseBodySizeStrict(const std::string& token) {
    if (token.empty() || !std::isdigit(static_cast<unsigned char>(token[0])))
        throw std::invalid_argument("error: bad client_max_body_size: " + token);

    uint64_t value = 0;
    const char* first = token.data();
    const char* last = token.data() + token.size();
    const auto result = std::from_chars(first, last, value, 10);
    if (result.ec != std::errc() || result.ptr == first)
        throw std::invalid_argument("error: bad client_max_body_size: " + token);

    if (result.ptr == last)
        return value;
    if (result.ptr + 1 != last)
        throw std::invalid_argument("error: bad client_max_body_size: " + token);

    const char unit = *result.ptr;
    auto scale = [&](uint64_t factor) -> uint64_t {
        if (value > std::numeric_limits<uint64_t>::max() / factor)
            throw std::invalid_argument("error: bad client_max_body_size: " + token);
        return value * factor;
    };
    if (unit == 'K' || unit == 'k')
        return scale(1024ULL);
    if (unit == 'M' || unit == 'm')
        return scale(1024ULL * 1024ULL);
    if (unit == 'G' || unit == 'g')
        return scale(1024ULL * 1024ULL * 1024ULL);
    throw std::invalid_argument("error: bad client_max_body_size: " + token);
}

const std::string& requireArg(const std::vector<std::string>& tokens, size_t& index,
                              const char* directive) {
    ++index;
    if (index >= tokens.size() || tokens[index] == "}" || tokens[index] == ";")
        throw std::invalid_argument(std::string("error: bad argument for '") + directive + "'");
    return tokens[index];
}

int parseStatusCode(const std::string& token, const char* context) {
    int code = 0;
    try {
        code = std::stoi(token);
    } catch (const std::out_of_range&) {
        throw std::invalid_argument(std::string("error: status code out of range in ") + context +
                                    ": " + token);
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument(std::string("error: bad status code in ") + context + ": " +
                                    token);
    }
    if (code < 100 || code > 599)
        throw std::invalid_argument(std::string("error: status code out of range in ") + context +
                                    ": " + token);
    return code;
}

}  // namespace

void ConfigParser::parse(const std::string& filename) {
    std::string file_data = readFile(filename);
    std::vector<std::string> tokens = tokenize(file_data);
    size_t index = 0;
    while (index < tokens.size()) {
        if (tokens[index] == "server") {
            parseServer(tokens, index);
        } else {
            throw std::invalid_argument("error: unexpected token outside server: " + tokens[index]);
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
        throw std::runtime_error("error: empty filename");

    std::ifstream file(filename.c_str());
    if (!file.is_open())
        throw std::runtime_error("error: open file: " + filename);

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
        } else if (std::isspace(static_cast<unsigned char>(input[i]))) {
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
    if (index >= tokens.size() || tokens[index] != "{")
        throw std::invalid_argument("error: expected '{' after server");
    index++;
    while (index < tokens.size() && tokens[index] != "}") {
        if (tokens[index] == "location") {
            parseLocation(tokens, index, new_server);
        } else {
            parseServerKeyword(tokens, index, new_server);
        }
    }
    if (index >= tokens.size() || tokens[index] != "}")
        throw std::invalid_argument("error: expected '}' after server");
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
        throw std::invalid_argument("error: expected path after location");
    new_location.setPath(tokens[index]);
    index++;
    if (index >= tokens.size() || tokens[index] != "{")
        throw std::invalid_argument("error: expected '{' after location path");
    index++;
    while (index < tokens.size() && tokens[index] != "}") {
        parseLocationKeyword(tokens, index, new_location);
    }
    if (index >= tokens.size() || tokens[index] != "}")
        throw std::invalid_argument("error: expected '}' after location");
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
        server.addPort(parsePortStrict(requireArg(tokens, index, "listen")));
        index++;
    } else if (found == "host") {
        server.setHost(requireArg(tokens, index, "host"));
        index++;
    } else if (found == "server_name") {
        index++;
        while (index < tokens.size() && tokens[index] != ";") {
            server.addServerName(tokens[index]);
            index++;
        }
    } else if (found == "client_max_body_size") {
        server.setClientMaxBodySize(
            parseBodySizeStrict(requireArg(tokens, index, "client_max_body_size")));
        index++;
    } else if (found == "error_page") {
        index++;
        std::vector<std::string> args;

        while (index < tokens.size() && tokens[index] != ";") {
            args.push_back(tokens[index]);
            index++;
        }
        if (args.size() < 2)
            throw std::invalid_argument("error: error_page requires status code and a file path");
        std::string file_path = args.back();
        args.pop_back();
        for (size_t i = 0; i < args.size(); i++) {
            const int code = parseStatusCode(args[i], "error_page");
            server.addErrorPagePath(static_cast<uint16_t>(code), file_path);
        }
    } else {
        throw std::invalid_argument("error: unknown server keyword: " + tokens[index]);
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
        const std::string& statusTok = requireArg(tokens, index, "return");
        const uint16_t status_code = static_cast<uint16_t>(parseStatusCode(statusTok, "return"));
        index++;
        std::string target_url;
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
    } else if (found == "client_max_body_size") {
        location.setClientMaxBodySize(
            parseBodySizeStrict(requireArg(tokens, index, "client_max_body_size")));
        index++;
    } else {
        throw std::invalid_argument("error: unknown location keyword: " + tokens[index]);
    }

    if (index < tokens.size() && tokens[index] == ";")
        index++;
}
