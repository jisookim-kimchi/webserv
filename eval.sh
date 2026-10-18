# 1. server start
./webserv configs/eval.config

# 2. 404 custom error page test
curl -i http://127.0.0.1:8080/nonexistent

# 3. body size limit (1MB above transfer 413 test)
python3 -c "import urllib.request; req = urllib.request.Request('http://127.0.0.1:8080/upload', data=b'A'*1100000, method='POST'); urllib.request.urlopen(req)"

# 4. method allow / deny test
curl -i -X DELETE http://127.0.0.1:8080/                # 405 Method Not Allowed
curl -i -X POST -d "test" http://127.0.0.1:8080/upload/f.txt  # 201 Created
curl -i http://127.0.0.1:8080/upload/f.txt             # 200 OK
curl -i -X DELETE http://127.0.0.1:8080/upload/f.txt   # 204 No Content

# 5. redirection and directory listing
curl -i http://127.0.0.1:8080/redirect                 # 301 Moved Permanently
curl -i http://127.0.0.1:8080/upload/                  # autoindex HTML print

# 6. multi port (other site)
curl -i http://127.0.0.1:8080/                         # port 1
curl -i http://127.0.0.1:8081/                         # port 2

# 7. CGI test (GET, POST, relative path)
curl -i http://127.0.0.1:8080/cgi-bin/hello.py
curl -i -X POST -d "greeting=hi" http://127.0.0.1:8080/cgi-bin/echo.py
curl -i http://127.0.0.1:8080/cgi-bin/relative.py
