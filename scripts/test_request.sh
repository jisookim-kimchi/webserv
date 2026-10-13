#!/usr/bin/env bash
set -e

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

mkdir -p build

c++ -std=c++17 -Wall -Wextra -Werror -Iincludes -Itests \
    srcs/HttpRequest.cpp \
    srcs/Utils.cpp \
    tests/test_http_request.cpp \
    -o build/test_req

./build/test_req
