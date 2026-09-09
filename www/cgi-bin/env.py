#!/usr/bin/env python3
"""Dump selected CGI environment variables (useful for integration checks)."""

import os
import sys


KEYS = [
    "GATEWAY_INTERFACE",
    "SERVER_PROTOCOL",
    "REQUEST_METHOD",
    "REQUEST_URI",
    "QUERY_STRING",
    "SCRIPT_NAME",
    "SCRIPT_FILENAME",
    "PATH_INFO",
    "DOCUMENT_ROOT",
    "SERVER_NAME",
    "SERVER_PORT",
    "REMOTE_ADDR",
    "CONTENT_TYPE",
    "CONTENT_LENGTH",
    "HTTP_HOST",
    "HTTP_USER_AGENT",
]


def main() -> int:
    lines = [f"{key}={os.environ.get(key, '')}" for key in KEYS]
    body = "\n".join(lines) + "\n"
    sys.stdout.write("Content-Type: text/plain\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.write(body)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
