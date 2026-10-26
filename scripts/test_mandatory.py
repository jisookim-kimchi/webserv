#!/usr/bin/env python3
import urllib.request
import urllib.error

tests = [
    ("GET /", "http://127.0.0.1:8080/", "GET", None, 200),
    ("POST /upload/test.txt", "http://127.0.0.1:8080/upload/test.txt", "POST", b"hello", 201),
    ("DELETE /upload/test.txt", "http://127.0.0.1:8080/upload/test.txt", "DELETE", None, 204),
    ("POST /get_only", "http://127.0.0.1:8080/get_only", "POST", b"foo", 405),
    ("GET /post_only", "http://127.0.0.1:8080/post_only", "GET", None, 405),
    ("GET /delete_only", "http://127.0.0.1:8080/delete_only", "GET", None, 405),
    ("POST /limit/big.txt (>10 bytes)", "http://127.0.0.1:8080/limit/big.txt", "POST", b"123456789012345", 413),
    ("POST /limit/small.txt (<=10 bytes)", "http://127.0.0.1:8080/limit/small.txt", "POST", b"12345", 201),
    ("GET /upload/ (autoindex on)", "http://127.0.0.1:8080/upload/", "GET", None, 200),
    ("GET /no_autoindex/ (autoindex off)", "http://127.0.0.1:8080/no_autoindex/", "GET", None, 404),
    ("GET /cgi-bin/hello.py?name=42", "http://127.0.0.1:8080/cgi-bin/hello.py?name=42", "GET", None, 200),
    ("POST /cgi-bin/echo.py", "http://127.0.0.1:8080/cgi-bin/echo.py", "POST", b"echo payload", 200),
    ("GET Port 8081 (virtual host)", "http://127.0.0.1:8081/", "GET", None, 200),
]

all_passed = True
for name, url, method, data, expected in tests:
    req = urllib.request.Request(url, data=data, method=method)
    try:
        with urllib.request.urlopen(req) as resp:
            code = resp.status
    except urllib.error.HTTPError as e:
        code = e.code
    except Exception as e:
        code = str(e)
    passed = (code == expected)
    if not passed:
        all_passed = False
    print(f"[{'PASS' if passed else 'FAIL'}] {name:<36} -> status: {code} (expected {expected})")

# Test Redirection 301 without following redirect
class NoRedirectHandler(urllib.request.HTTPRedirectHandler):
    def redirect_request(self, req, fp, code, msg, headers, newurl):
        return None

opener = urllib.request.build_opener(NoRedirectHandler)
try:
    resp = opener.open("http://127.0.0.1:8080/redirect")
    redir_code = resp.status
    location = resp.headers.get("Location")
except urllib.error.HTTPError as e:
    redir_code = e.code
    location = e.headers.get("Location")

passed = (redir_code == 301 and location == "http://nginx.org")
if not passed:
    all_passed = False
print(f"[{'PASS' if passed else 'FAIL'}] {'Redirect 301':<36} -> status: {redir_code}, Location: {location}")

if all_passed:
    print("\nALL MANDATORY TEST CASES PASSED SUCCESSFULLY!")
else:
    print("\nSOME TESTS FAILED!")
