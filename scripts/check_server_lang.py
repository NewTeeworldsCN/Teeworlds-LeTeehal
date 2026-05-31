#!/usr/bin/env python3
"""Report server _() keys missing from server_lang/en.json."""

from __future__ import annotations

import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SERVER_SRC = ROOT / "src" / "game" / "server"
EN_JSON = ROOT / "server_lang" / "en.json"

LOCALIZE_RE = re.compile(r'_\(\s*"((?:[^"\\]|\\.)*)"\s*\)')


def unescape_c_string(s: str) -> str:
    out: list[str] = []
    i = 0
    while i < len(s):
        if s[i] == "\\" and i + 1 < len(s):
            n = s[i + 1]
            if n == "n":
                out.append("\n")
            elif n == "t":
                out.append("\t")
            elif n == "r":
                out.append("\r")
            elif n in ('\\', '"'):
                out.append(n)
            else:
                out.append(s[i])
            i += 2
        else:
            out.append(s[i])
            i += 1
    return "".join(out)


def scan_keys() -> set[str]:
    keys: set[str] = set()
    for path in SERVER_SRC.rglob("*"):
        if path.suffix not in (".cpp", ".h"):
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        for m in LOCALIZE_RE.finditer(text):
            keys.add(unescape_c_string(m.group(1)))
    return keys


def main() -> int:
    if not EN_JSON.is_file():
        print(f"Missing {EN_JSON}", file=sys.stderr)
        return 2
    en_data = json.loads(EN_JSON.read_text(encoding="utf-8"))
    en = {e["key"] for e in en_data.get("translation", [])}
    keys = scan_keys()
    missing = sorted(k for k in keys if k not in en)
    if missing:
        print(f"Missing {len(missing)} key(s) in en.json:")
        for k in missing[:50]:
            print(f"  {k!r}")
        if len(missing) > 50:
            print(f"  ... and {len(missing) - 50} more")
        return 1
    print(f"OK: {len(keys)} localize keys covered in en.json")
    return 0


if __name__ == "__main__":
    sys.exit(main())
