#!/usr/bin/env bash
set -e

mkdir -p build

c++ -std=c++17 -Wall -Wextra -Werror -Iincludes -Itests \
    srcs/HttpRequest.cpp \
    srcs/Utils.cpp \
    tests/test_http_request.cpp \
    -o build/test_req

./build/test_req
