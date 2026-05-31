#!/usr/bin/env python3
"""Legacy wrapper: regenerate server_lang/en.json from source strings."""

import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    build_map = ROOT / "scripts" / "build_full_en_map.py"
    generate = ROOT / "scripts" / "generate_server_lang.py"

    if not generate.exists():
        print("Error: scripts/generate_server_lang.py not found")
        return 1

    if build_map.exists():
        subprocess.run([sys.executable, str(build_map)], cwd=ROOT, check=True)

    subprocess.run([sys.executable, str(generate)], cwd=ROOT, check=True)
    print("Updated server_lang/en.json and server_lang/cn.json")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
