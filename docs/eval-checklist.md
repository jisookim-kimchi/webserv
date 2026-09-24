# Peer-evaluation checklist (EvalHub + subject)

Run on **Linux** (requires `epoll`). From the project root:

```bash
make re
./webserv configs/eval.config
```

## Mandatory smoke tests

| Check | Command / action |
|-------|------------------|
| Static GET | Browser or `curl -i http://127.0.0.1:8080/` |
| Second port | `curl -i http://127.0.0.1:8081/` |
| 404 page | `curl -i http://127.0.0.1:8080/no-such-page` |
| Autoindex | `curl -i http://127.0.0.1:8080/upload/` |
| Redirect | `curl -i http://127.0.0.1:8080/redirect` |
| POST upload | `curl -i -X POST --data-binary 'hello' http://127.0.0.1:8080/upload/` |
| DELETE | `curl -i -X DELETE http://127.0.0.1:8080/upload/UPLOAD_NAME` |
| Body limit | `curl -i -X POST --data-binary @large_file http://127.0.0.1:8080/upload/` (expect 413 if over `client_max_body_size`) |
| UNKNOWN method | `curl -i -X FOO http://127.0.0.1:8080/` (must not crash) |
| CGI GET | `curl -i 'http://127.0.0.1:8080/cgi-bin/hello.py?name=42'` |
| CGI POST | `curl -i -X POST --data-binary 'x' http://127.0.0.1:8080/cgi-bin/echo.py` |
| CGI error / slow | `curl -i http://127.0.0.1:8080/cgi-bin/slow.py` (timeout, server stays up) |
| Duplicate listen | Config with two servers on same `host:port` must fail at startup |

## Siege stress

```bash
sudo apt-get install -y siege
# empty-ish page under load
siege -b -c 50 -t 30S http://127.0.0.1:8080/
```

Target: availability **≥ 99.5%**, memory not growing without bound, no hanging connections.
Client idle timeout is **60s** (incomplete / silent clients are closed).

## Memory / leak checks

On Linux (Codespace or CI):

```bash
chmod +x scripts/memcheck.sh
./scripts/memcheck.sh unit      # AddressSanitizer + leak detection on unit tests
./scripts/memcheck.sh valgrind  # Valgrind on live ./webserv + curl traffic
./scripts/memcheck.sh rss       # RSS sampling under load (should stay flat)
```

Or trigger the GitHub Action **Memory check** (`workflow_dispatch` / PR).

On macOS, full server Valgrind is unavailable (no `epoll`); use Codespace/CI for leak proof.

## Unit tests

```bash
./test.sh all
```

## Notes for defense

- One `epoll_wait` watches listen fds, client fds, and CGI pipes.
- Never use `errno` after `read`/`recv`/`write`/`send` to steer control flow.
- `fcntl` uses only `F_SETFL` + `O_NONBLOCK`.
- Language standard: **C++17** (`-std=c++17`).
