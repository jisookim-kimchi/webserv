#!/usr/bin/env python3
"""Intentionally slow CGI used by timeout tests."""

import sys
import time


def main() -> int:
    time.sleep(30)
    sys.stdout.write("Content-Type: text/plain\r\n\r\n")
    sys.stdout.write("too late\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
