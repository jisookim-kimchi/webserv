*This project has been created as part of the 42 curriculum by weiyuandu, jisookim.*

# webserv

## Description

`webserv` is an HTTP/1.1 server written in C++17. It serves static files, handles
file upload and deletion, optional directory listing (autoindex), HTTP redirects,
and CGI scripts (for example Python) through a single non-blocking `epoll` event
loop. Listening sockets, client sockets, and CGI pipes are all driven by the same
`epoll_wait` call.

## Instructions

### Build

```bash
make
```

Requires a Linux environment with `epoll` (the evaluation target). macOS does not
provide `sys/epoll.h`, so the full binary will not build there.

### Run

```bash
./webserv [configuration file]
```

If no configuration file is given, the default is `configs/test.config`.

For a multi-port evaluation demo:

```bash
./webserv configs/eval.config
```

Then open `http://127.0.0.1:8080/` in a browser, or use `curl` against ports
`8080` and `8081`.

### Tests

```bash
./test.sh all
```

Individual modules: `./test.sh config`, `./test.sh cgi`, `./test.sh http_request`,
`./test.sh http_response`.

### Stress (on Linux)

See [docs/eval-checklist.md](docs/eval-checklist.md) for Siege and peer-evaluation steps.

## Resources

- [RFC 2616 / HTTP/1.1](https://www.rfc-editor.org/rfc/rfc2616) (reference subset)
- [NGINX Beginner’s Guide](https://nginx.org/en/docs/beginners_guide.html) (config behaviour comparison)
- [epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html)
- [CGI /1.1](https://datatracker.ietf.org/doc/html/rfc3875)

### AI use

AI assistants were used to draft and refactor modules (config parsing, HTTP
request/response, CGI epoll wiring), to prepare evaluation hardening (timeouts,
README, errno/`fcntl` compliance), and to write unit tests and documentation.
All generated code was reviewed, tested, and adapted by the authors before
inclusion.
