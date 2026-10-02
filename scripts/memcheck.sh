#!/usr/bin/env bash
# Memory / leak checks for webserv.
# - On macOS: AddressSanitizer on unit-test modules (no epoll).
# - On Linux: ASan unit tests + optional Valgrind on the live server.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

CXX="${CXX:-c++}"
OS="$(uname -s)"
MODE="${1:-all}"

SAN_FLAGS=(-Wall -Wextra -Werror -g -std=c++17 -Iincludes -Itests -fsanitize=address -fno-omit-frame-pointer)
# Leak detection via ASan only works on Linux/glibc. On Darwin, use AddressSanitizer
# for memory errors and the system `leaks` tool for heap growth at exit.
if [[ "$(uname -s)" == "Darwin" ]]; then
  ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0:halt_on_error=1:abort_on_error=1}"
else
  ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1:halt_on_error=1:abort_on_error=1}"
fi
export ASAN_OPTIONS

mkdir -p build

build_and_run() {
  local name="$1"
  shift
  echo "[asan] build $name"
  "$CXX" "${SAN_FLAGS[@]}" "$@" -o "build/${name}_asan"
  echo "[asan] run   $name"
  "./build/${name}_asan"
}

run_unit_asan() {
  # Config tests fork+exec heavily; ASan+fork is flaky — still run, but last.
  build_and_run test_http_request \
    tests/test_HTTP_request.cpp \
    srcs/HTTP_request.cpp

  build_and_run test_http_response \
    tests/test_http_response.cpp \
    srcs/HttpResponse.cpp srcs/LocationConfig.cpp srcs/ServerConfig.cpp srcs/Utils.cpp

  build_and_run test_cgi \
    tests/test_cgi.cpp \
    srcs/CgiHandler.cpp srcs/Utils.cpp

  build_and_run test_config \
    tests/test_config.cpp \
    srcs/ConfigParser.cpp srcs/LocationConfig.cpp srcs/ServerConfig.cpp

  echo
  echo "ASan unit tests: OK"
}

run_server_valgrind() {
  if [[ "$OS" != "Linux" ]]; then
    echo "Valgrind server check requires Linux (epoll). Skipping."
    return 0
  fi
  if ! command -v valgrind >/dev/null 2>&1; then
    echo "valgrind not installed. On Ubuntu: sudo apt-get install -y valgrind"
    return 1
  fi

  echo "[valgrind] build webserv (no sanitize, for valgrind)"
  make re
  mkdir -p build

  local conf="${MEMCHECK_CONFIG:-configs/eval.config}"
  local log="build/valgrind-webserv.log"
  local err="build/webserv-stderr.log"
  echo "[valgrind] start ./webserv $conf (log: $log)"
  # Valgrind startup is slow; do not curl until the listen port answers.
  valgrind --leak-check=full --show-leak-kinds=definite,possible \
    --errors-for-leak-kinds=definite --error-exitcode=42 \
    --log-file="$log" \
    ./webserv "$conf" >build/webserv-stdout.log 2>"$err" &
  local pid=$!

  local ready=0
  local i
  for i in $(seq 1 90); do
    if ! kill -0 "$pid" 2>/dev/null; then
      echo "webserv exited before accepting connections"
      echo "---- stderr ----"
      cat "$err" || true
      echo "---- valgrind ----"
      cat "$log" || true
      return 1
    fi
    if curl -sS -o /dev/null --connect-timeout 1 "http://127.0.0.1:8080/" 2>/dev/null; then
      ready=1
      echo "[valgrind] server ready after ${i}s"
      break
    fi
    sleep 1
  done

  if [[ "$ready" -ne 1 ]]; then
    echo "webserv did not become ready on :8080 within 90s"
    echo "---- stderr ----"
    cat "$err" || true
    echo "---- valgrind (partial) ----"
    tail -80 "$log" || true
    kill -TERM "$pid" 2>/dev/null || true
    wait "$pid" 2>/dev/null || true
    return 1
  fi

  echo "[valgrind] traffic..."
  curl -sS -o /dev/null "http://127.0.0.1:8080/" || true
  curl -sS -o /dev/null "http://127.0.0.1:8080/no-such" || true
  curl -sS -o /dev/null "http://127.0.0.1:8080/upload/" || true
  curl -sS -o /dev/null -X POST --data-binary 'leak-test' "http://127.0.0.1:8080/upload/" || true
  curl -sS -o /dev/null "http://127.0.0.1:8080/cgi-bin/hello.py?name=42" || true
  curl -sS -o /dev/null -X POST --data-binary 'x' "http://127.0.0.1:8080/cgi-bin/echo.py" || true
  curl -sS -o /dev/null "http://127.0.0.1:8081/" || true

  # Graceful stop so Valgrind can dump on exit
  kill -TERM "$pid" 2>/dev/null || kill "$pid" 2>/dev/null || true
  wait "$pid" || true

  echo "[valgrind] summary:"
  grep -E "ERROR SUMMARY|definitely lost|indirectly lost|possibly lost|still reachable" "$log" || cat "$log"
  if grep -q "definitely lost: 0 bytes" "$log" && grep -q "ERROR SUMMARY: 0 errors" "$log"; then
    echo "Valgrind: no definite leaks / errors"
    return 0
  fi
  echo "Valgrind reported issues — see $log"
  return 1
}

run_rss_watch() {
  if [[ "$OS" != "Linux" ]]; then
    echo "RSS watch requires Linux webserv binary. Skipping."
    return 0
  fi
  make -s
  local conf="${MEMCHECK_CONFIG:-configs/eval.config}"
  ./webserv "$conf" &
  local pid=$!
  sleep 1
  echo "pid=$pid  sampling RSS under curl load (20s)..."
  for i in $(seq 1 10); do
    rss=$(ps -o rss= -p "$pid" 2>/dev/null | tr -d ' ' || echo 0)
    echo "t=${i}0s rss_kb=$rss"
    for _ in $(seq 1 20); do
      curl -sS -o /dev/null "http://127.0.0.1:8080/" || true
    done
    sleep 2
  done
  kill -INT "$pid" 2>/dev/null || kill "$pid" 2>/dev/null || true
  wait "$pid" || true
  echo "RSS should stay roughly flat (not climb without bound)."
}

case "$MODE" in
  unit|asan)
    run_unit_asan
    ;;
  valgrind|server)
    run_server_valgrind
    ;;
  rss)
    run_rss_watch
    ;;
  all)
    run_unit_asan
    if [[ "$OS" == "Linux" ]]; then
      run_server_valgrind || true
      run_rss_watch || true
    else
      echo
      echo "Note: full server Valgrind/RSS checks need Linux (Codespace/CI)."
      echo "  ./scripts/memcheck.sh valgrind"
      echo "  ./scripts/memcheck.sh rss"
    fi
    ;;
  *)
    echo "usage: $0 [all|unit|valgrind|rss]" >&2
    exit 1
    ;;
esac
