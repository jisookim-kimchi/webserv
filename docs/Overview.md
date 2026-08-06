# Webserv Overview

This document describes the modular architecture for this project. The goal is to separate configuration parsing, network listening, connection management, HTTP processing, and CGI into independent modules, reducing coupling and making development, testing, and debugging easier.

## Overall Directory Structure

```plaintext
webserv/
├── Makefile
├── includes/
│   ├── Config.hpp
│   ├── Server.hpp
│   ├── Client.hpp
│   ├── HttpRequest.hpp
│   ├── HttpResponse.hpp
│   ├── CgiHandler.hpp
│   └── Utils.hpp
├── srcs/
│   ├── main.cpp
│   ├── Config.cpp
│   ├── Server.cpp
│   ├── Client.cpp
│   ├── HttpRequest.cpp
│   ├── HttpResponse.cpp
│   ├── CgiHandler.cpp
│   └── Utils.cpp
├── config/
│   └── default.conf
└── www/
    ├── index.html
    ├── 404.html
    ├── upload/
    └── cgi-bin/
```

## Module Responsibilities

### `main.cpp`

Program entry point, responsible for:

- Checking command-line arguments and obtaining the configuration file path.
- Registering global signal handling, such as ignoring `SIGPIPE`, to prevent the process from crashing when a client disconnects unexpectedly.
- Creating a `Config` object and parsing the configuration file.
- Passing the parsed result to `Server` and starting the main event loop.

### `Config.hpp` / `Config.cpp`

Responsible for configuration parsing and storage. The core data structures include:

- `LocationConfig`: stores the configuration for a single `location` block.
  - Allowed methods.
  - `root`, `index`, and `autoindex`.
  - Redirect settings.
  - CGI mapping.
  - Upload path.
- `ServerConfig`: stores the configuration for a single `server` block.
  - Listening address and port.
  - `server_name`.
  - Default error pages.
  - `client_max_body_size`.
  - A list of `LocationConfig` entries.
- `Config`: reads the `.conf` file, performs lexical and syntax parsing, and generates multiple `ServerConfig` objects.

### `Server.hpp` / `Server.cpp`

Core of the server engine, responsible for:

- Socket initialization: `socket` -> `setsockopt` -> `fcntl(O_NONBLOCK)` -> `bind` -> `listen`.
- Maintaining the main event loop `run()`, using `poll()` to manage all listening sockets and client sockets.
- Dispatching events:
  - When a listening socket becomes readable, call `accept()` to establish a new connection.
  - When a client socket becomes readable or writable, hand it over to the corresponding `Client` instance.
- Reaping timed-out or disconnected connections, closing file descriptors, and cleaning up resources.

### `Client.hpp` / `Client.cpp`

Encapsulates a single client connection and its connection state machine, responsible for:

- Storing the client socket file descriptor.
- Maintaining input/output buffers: `_readBuffer` and `_writeBuffer`.
- Tracking connection states such as `READING_HEADER`, `READING_BODY`, `PROCESSING`, and `WRITING`.
- Holding request and response objects, and providing `handleRead()` and `handleWrite()` for `Server` to call.

### `HttpRequest.hpp` / `HttpRequest.cpp`

Incrementally parses HTTP requests from the client's input buffer, responsible for:

- Parsing the request line: method, URL, and version.
- Parsing headers: `Host`, `Content-Length`, `Transfer-Encoding`, and so on.
- Parsing the body: normal bodies, `multipart/form-data` uploads, and chunked transfer decoding.
- Maintaining parsing state to handle partial and coalesced packets correctly, avoiding premature request completion.

### `HttpResponse.hpp` / `HttpResponse.cpp`

Generates HTTP responses based on the request and matched configuration, responsible for:

- Validating whether the request method is allowed and whether the body exceeds limits.
- Reading static files and building the correct response headers, such as `Content-Type` and `Content-Length`.
- Handling directory requests: looking for an `index` file; if none exists and `autoindex` is enabled, generating a directory listing page.
- Generating error responses such as `404`, `405`, `413`, and `500`, and loading the corresponding error pages.
- Writing the final response data into the client's output buffer.

### `CgiHandler.hpp` / `CgiHandler.cpp`

When a request matches a CGI resource, this module executes the script and collects the result:

- Building the `envp` environment variable array required by `execve`.
- Creating a `pipe` and starting a child process with `fork`.
- Redirecting `STDIN` and `STDOUT` to feed the request body and read the script output.
- Registering the pipe file descriptors in the event loop to achieve non-blocking CGI interaction.

### `Utils.hpp` / `Utils.cpp`

Contains shared utility functions such as:

- String handling.
- Path joining and normalization.
- Setting file descriptors to non-blocking mode.
- Other small helper functions reused across multiple modules.

## Recommended Data Flow

1. `main.cpp` reads the configuration and starts `Server`.
2. `Server` listens on the configured ports, accepts new connections, and creates `Client` objects.
3. `Client` reads request data and passes it to `HttpRequest` for parsing.
4. After parsing is complete, `HttpResponse` generates the response based on the request and configuration.
5. If the request matches CGI, `CgiHandler` executes the script and collects its output.
6. `Client` writes the response back to the socket, and `Server` reclaims the connection at the appropriate time.

## Design Principles

- Keep configuration, networking, protocol parsing, response generation, and CGI handling independent.
- Expose stable interfaces whenever possible and avoid direct dependencies on internal implementation details.
- Make each module responsible for a single concern to simplify unit testing and debugging.
- Keep the implementation compliant with C++98 and avoid introducing unnecessary modern syntax or library features.

## Directory Conventions

- `config/`: stores the default configuration file and example configurations.
- `www/`: stores static assets, error pages, upload directories, and CGI scripts.
- `tests/`: stores parser test inputs, sample outputs, and validation scripts.
