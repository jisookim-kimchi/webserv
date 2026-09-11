#!/usr/bin/env python3
"""CGI that returns a custom Status header for redirect tests."""

import sys


def main() -> int:
    body = "redirect body"
    sys.stdout.write("Status: 302 Found\r\n")
    sys.stdout.write("Location: /elsewhere\r\n")
    sys.stdout.write("Content-Type: text/plain\r\n")
    sys.stdout.write(f"Content-Length: {len(body)}\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.write(body)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
