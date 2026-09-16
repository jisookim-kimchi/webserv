#!/usr/bin/env python3
"""Echo request method, query string, and body back as plain text."""

import os
import sys


def main() -> int:
    body = sys.stdin.read()
    sys.stdout.write("Content-Type: text/plain\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.write("CGI_OK\n")
    sys.stdout.write(f"REQUEST_METHOD={os.environ.get('REQUEST_METHOD', '')}\n")
    sys.stdout.write(f"QUERY_STRING={os.environ.get('QUERY_STRING', '')}\n")
    sys.stdout.write(f"CONTENT_LENGTH={os.environ.get('CONTENT_LENGTH', '')}\n")
    sys.stdout.write(f"CONTENT_TYPE={os.environ.get('CONTENT_TYPE', '')}\n")
    sys.stdout.write(f"SCRIPT_NAME={os.environ.get('SCRIPT_NAME', '')}\n")
    sys.stdout.write(f"BODY={body}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
