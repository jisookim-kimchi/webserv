#!/usr/bin/env bash
set -e

mkdir -p build

c++ -std=c++17 -Wall -Wextra -Werror -Iincludes -Itests \
    srcs/HTTP_request.cpp \
    srcs/Utils.cpp \
    tests/test_HTTP_request.cpp \
    -o build/test_req

./build/test_req
