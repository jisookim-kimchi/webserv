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
    std::ofstream outfile("tests/test_result.txt");

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



