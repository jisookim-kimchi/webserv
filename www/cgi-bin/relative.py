#!/usr/bin/env python3
"""CGI that reads relative files from its own directory (chdir context check)."""

import pathlib
import sys


def main() -> int:
    marker = pathlib.Path("relative_marker.txt")
    content = marker.read_text(encoding="utf-8").strip() if marker.exists() else "missing"
    body = f"RELATIVE_OK={content}\n"
    sys.stdout.write("Content-Type: text/plain\r\n")
    sys.stdout.write("\r\n")
    sys.stdout.write(body)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
