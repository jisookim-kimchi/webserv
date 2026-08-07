#!/bin/bash
c++ -Wall -Wextra -Werror -g -std=c++17 tests/main.cpp srcs/*.cpp \
<<<<<<< HEAD
-Iincludes -o test_parser && ./test_parser configs/Basic.config
=======
-Iincludes -o test_socket
>>>>>>> feature/socket-bind
