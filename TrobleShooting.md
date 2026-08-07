1. Accept Error return 11.
    - errno : 11 (EAGAIN or EWOULDBLOCK)
        - there's no client requests in non-blocking mode so try it again 