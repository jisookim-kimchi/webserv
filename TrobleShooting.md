
1. Because listening socket is in non-blocking mode, accept() returns -1 and sets errno to EAGAIN or EWOULDBLOCK if there are no pending connections.
    - errno : 11 (EAGAIN or EWOULDBLOCK)
        - there's no client requests in non-blocking mode so try it again

### Screenshots
![accept error 11 - terminal output](./Trouble/webserv_accept_error_11_1.jpg)
![accept error 11 - gdb debug](./Trouble/webserv_accept_error_11_2.jpg)


