#!/bin/bash
c++ -Wall -Wextra -Werror -g -std=c++17 tests/main.cpp srcs/*.cpp \
-Iincludes -o test_socket