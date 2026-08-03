#include "../includes/ConfigParser.hpp"
#include <iostream>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "error : need to input config file" << std::endl;
        return 1;
    }
    ConfigParser parser;
    parser.parse(argv[1]);
    

    return 0;
}