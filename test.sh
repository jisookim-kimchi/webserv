#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
cd "$ROOT"

CXX="${CXX:-c++}"
CXXFLAGS=(-Wall -Wextra -Werror -g -std=c++17 -Iincludes -Itests)
mkdir -p build

MODULE="${1:-all}"

build_config() {
  echo "[build] test_config"
  "$CXX" "${CXXFLAGS[@]}" \
    tests/test_config.cpp \
    srcs/ConfigParser.cpp srcs/LocationConfig.cpp srcs/ServerConfig.cpp \
    -o build/test_config
}

build_cgi() {
  echo "[build] test_cgi"
  "$CXX" "${CXXFLAGS[@]}" \
    tests/test_cgi.cpp \
    srcs/CgiHandler.cpp srcs/Utils.cpp \
    -o build/test_cgi
}

build_http_request() {
  echo "[build] test_http_request"
  "$CXX" "${CXXFLAGS[@]}" \
    tests/test_HTTP_request.cpp \
    srcs/HTTP_request.cpp \
    -o build/test_http_request
}

run_config() {
  echo "[run]   test_config"
  ./build/test_config
}

run_cgi() {
  echo "[run]   test_cgi"
  ./build/test_cgi
}

run_http_request() {
  echo "[run]   test_http_request"
  ./build/test_http_request
}

case "${MODULE}" in
  all)
    build_config
    build_cgi
    build_http_request
    run_config
    run_cgi
    run_http_request
    echo
    echo "All tests passed."
    ;;
  config)
    build_config
    run_config
    ;;
  cgi)
    build_cgi
    run_cgi
    ;;
  http_request|request)
    build_http_request
    run_http_request
    ;;
  *)
    echo "usage: $0 [all|config|cgi|http_request]" >&2
    exit 1
    ;;
esac
