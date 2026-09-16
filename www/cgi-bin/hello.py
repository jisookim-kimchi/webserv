#!/usr/bin/env python3
"""Simple GET CGI used by the landing page and smoke tests."""

import os
import sys


def main() -> int:
    name = os.environ.get("QUERY_STRING", "")
    if name.startswith("name="):
        name = name[5:]
    if not name:
        name = "webserv"

    body = f"<html><body><h1>Hello, {name}!</h1></body></html>"
    sys.stdout.write("Content-Type: text/html\r\n")
    sys.stdout.write(f"Content-Length: {len(body.encode())}\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.write(body)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
