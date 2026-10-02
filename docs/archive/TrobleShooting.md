1. Because listening socket is in non-blocking mode, accept() returns -1 and sets errno to EAGAIN or EWOULDBLOCK if there are no pending connections.
    - errno : 11 (EAGAIN or EWOULDBLOCK)
        - there's no client requests in non-blocking mode so try it again

### Screenshots
![accept error 11 - terminal output](./Trouble/webserv_accept_error_11_1.jpg)
![accept error 11 - gdb debug](./Trouble/webserv_accept_error_11_2.jpg)

2. regarding FD

- Issue : FD is a Unique System Resource, so i blocked Copy Constructor and = operator for Server class.

so i set listenSockets_ = std::vector<std::unique_ptr<ListenSocket>>;
unique_ptr cannot copyable and assignable.
so we can use std::move for transferring ownership.


3. regarding CGI Path
- Issue : (`chdir` & relative path):
  - child process calls `chdir("www/cgi-bin")`, so passing relative `scriptPath` (`"www/cgi-bin/hello.py"`) makes Python look for `www/cgi-bin/www/cgi-bin/hello.py` (fails with `[Errno 2] No such file or directory`).
  - Fix: use `getcwd()` to pass the absolute path (`cwd + "/" + fullPath`).


4.  `Received HTTP/0.9 when not allowed`
- Issue :  `curl: (1) Received HTTP/0.9 when not allowed`
  when requesting `GET /cgi-bin/hello.py`, server sends raw CGI output directly `Content-Type`(raw text instead of HTTP format)
  ```
  HOOK: final packet :
  Content-Type: text/html
  Content-Length: 50
  ```
  this is not `HTTP Format`
- Fix : Wrapped CGI output with standard HTTP/1.1 status line (`HTTP/1.1 200 OK`) and `Content-Length` header to resolve the `HTTP/0.9` error.

5. Empty response after request parse
- Issue : after `HTTP_Request::parse` succeeded, `setResponseBuffer` was still commented / TODO, but the loop already switched the client to `EPOLLOUT`.
  `curl` got an empty body (or nothing useful) even though the server accepted the connection.

6. Incomplete body treated as a finished request
- Issue : as soon as `\r\n\r\n` appeared, `processRequest()` ran.
  For `Content-Length` / chunked POST, the body was often still incomplete → parse failed as 400, or the body was truncated.

7. Partial `send` dropped the rest of the response
- Issue : `handleClientWrite` called `send()` once and then closed the client.
  On a short write (common with non-blocking sockets / large CGI or file bodies), the client only received the first chunk.
  `Client::offset_` existed but was unused.

8. CGI script path did not match static file mapping
- Issue : CGI built the script path as `cwd + "/" + root + url` (and hardcoded `"www"` when location root was empty).
  Extension locations like `.py` often have no `root`, so the path became wrong vs `HttpResponse::mapUrlToFs` / longest prefix location → CGI 500 / file not found.

9. Build artifacts committed into git
- Issue : `objs/*.o` and the `webserv` binary were tracked in the branch.
  Polluted the PR diff, bloated the repo, and caused noisy merge / CI noise.
  