#include "../includes/ConfigParser.hpp"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <iostream>

ConfigParser::ConfigParser() {}
ConfigParser::~ConfigParser() {}
ConfigParser::ConfigParser(const ConfigParser& other) { *this = other; }
ConfigParser& ConfigParser::operator=(const ConfigParser& other)
{
    if (this != &other)
    {
        server_configs_ = other.server_configs_;
    }
    return *this;
}

void ConfigParser::parse(const std::string& filename)
{
    std::string file_data = readFile(filename);
    std::vector<std::string> tokens = tokenize(file_data);
    size_t index = 0;
    while (index < tokens.size())
    {
        if (tokens[index] == "server")
        {
            parseServer(tokens, index);
        }
        else
        {
            std::cout << "error : unexpected token outside server: " << tokens[index] << std::endl;
            return;
        }
    }

    std::ofstream outfile("tests/test_tokenize.txt");

    for (size_t i = 0; i < tokens.size(); ++i)
    {
        outfile << "[" << i << "] ----> " << tokens[i] << std::endl;
    }

    outfile.close();
}

/*
    @brief : read config file data
    @param1 : filename
    @return : string of file
    @throw : runtime_error if file is empty or not found
*/
std::string ConfigParser::readFile(const std::string &filename)
{
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
    @param1 : line of config file
    @return : vector of tokens
    @think : '/' case??
*/
std::vector<std::string> ConfigParser::tokenize(const std::string &line)
{
    std::vector<std::string> tokens;
    std::string token;
    for (size_t i = 0; i < line.length(); i++)
    {
        if (line[i] == '#')
        {
            if (!token.empty())
            {
                tokens.push_back(token);
                token.clear();
            }
            while (i < line.length() && line[i] != '\n')
                i++;
        }
        else if (isspace(line[i]))
        {
            if(!token.empty())
            {
                tokens.push_back(token);
                token.clear();
            }
        }
        else if (line[i] == '{' || line[i] == '}' || line[i] == ';')
        {
            if (!token.empty())
            {
                tokens.push_back(token);
                token.clear();
            }
            tokens.push_back(std::string(1, line[i]));
        }
        else
        {
            token += line[i];
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
void ConfigParser::parseServer(const std::vector<std::string>& tokens, size_t &index)
{
    ServerConfig new_server;
    index++;
    if (index >= tokens.size() || tokens[index] != "{")
    {
        std::cout << "error : expected '{' after server" << std::endl;
        return;
    }
    index++;
    while (index < tokens.size() && tokens[index] != "}")
    {
        if (tokens[index] == "location")
        {
           parseLocation(tokens, index, new_server);
        }
        else
        {
            parseServerKeyword(tokens, index, new_server);
        }
    }
    if (index >= tokens.size() || tokens[index] != "}")
    {
        std::cout << "error : expected '}' after server" << std::endl;
        return;
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
void ConfigParser::parseLocation(const std::vector<std::string>& tokens, size_t &index, ServerConfig& server)
{
    LocationConfig new_location;
    index++;
    new_location.setPath(tokens[index]);
    index++;
    if (index >= tokens.size() || tokens[index] != "{")
    {
        std::cout << "error : expected '{' after location path" << std::endl;
        return;
    }
    index++;
    while (index < tokens.size() && tokens[index] != "}")
    {
        parseLocationKeyword(tokens, index, new_location);
    }
    if (index >= tokens.size() || tokens[index] != "}")
    {
        std::cout << "error : expected '}' after location" << std::endl;
        return;
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
void ConfigParser::parseServerKeyword(const std::vector<std::string>& tokens, size_t& index, ServerConfig& server)
{
    const std::string& found = tokens[index];
    if (found == "listen")
    {
        index++;
        if (index < tokens.size())
            server.addPort(std::stoi(tokens[index]));
        index++;
    }
    else if (found == "host")
    {
        index++;
        if (index < tokens.size())
            server.setHost(tokens[index]);
        index++;
    }
    else if (found == "server_name")
    {
        index++;
        while (index < tokens.size() && tokens[index] != ";")
        {
            server.addServerName(tokens[index]);
            index++;
        }
    }
    else if (found == "client_max_body_size")
    {
        index++;
        if (index < tokens.size())
        {
            uint64_t cmbs = std::stoull(tokens[index]); 
            char unit = tokens[index].back();

            if (unit == 'K' || unit == 'k')
                cmbs *= 1024ULL;
            else if (unit == 'M' || unit == 'm')
                cmbs *= 1024ULL * 1024ULL;
            else if (unit == 'G' || unit == 'g')
                cmbs *= 1024ULL * 1024ULL * 1024ULL;
            server.setClientMaxBodySize(cmbs);
        }
        index++;
    }
    else if (found == "error_page")
    {
        index++;
        std::vector<std::string> args;
        
        while (index < tokens.size() && tokens[index] != ";")
        {
            args.push_back(tokens[index]);
            index++;
        }
        if (args.size() >= 2)
        {
            std::string file_path = args.back();
            args.pop_back();
            for (size_t i = 0; i < args.size(); i++)
            {
                server.addErrorPagePath(std::stoi(args[i]), file_path);
            }
        }
    }
    else
    {
        while (index < tokens.size() && tokens[index] != ";")
            index++;
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
void ConfigParser::parseLocationKeyword(const std::vector<std::string>& tokens, size_t& index, LocationConfig& location)
{
    (void)tokens;
    (void)location;
    index++;
}