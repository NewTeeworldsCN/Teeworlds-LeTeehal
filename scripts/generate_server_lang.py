#!/usr/bin/env python3
"""Scan server _() keys and regenerate server_lang/en.json and cn.json."""

from __future__ import annotations

import ast
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SERVER_SRC = ROOT / "src" / "game" / "server"
EN_JSON = ROOT / "server_lang" / "en.json"
CN_JSON = ROOT / "server_lang" / "cn.json"
EN_MAP_JSON = ROOT / "server_lang" / "en_map.json"
FIX_EN = ROOT / "scripts" / "fix_en_json.py"

LOCALIZE_RE = re.compile(r'_\(\s*"((?:[^"\\]|\\.)*)"\s*\)')
REGISTER_SCRAP_RE = re.compile(r'RegisterScrap\(\s*"([^"]+)"')
STORE_NAME_RE = re.compile(r'\{\s*"[^"]+",\s*"([^"]+)"\s*,')
CHINESE_RE = re.compile(r"[\u4e00-\u9fff]")


def unescape_c_string(s: str) -> str:
    out: list[str] = []
    i = 0
    while i < len(s):
        if s[i] == "\\" and i + 1 < len(s):
            n = s[i + 1]
            if n == "n":
                out.append("\n")
                i += 2
            elif n == "t":
                out.append("\t")
                i += 2
            elif n == "r":
                out.append("\r")
                i += 2
            elif n == "\\":
                out.append("\\")
                i += 2
            elif n == '"':
                out.append('"')
                i += 2
            else:
                out.append(s[i])
                i += 1
        else:
            out.append(s[i])
            i += 1
    return "".join(out)


def scan_localize_keys() -> set[str]:
    keys: set[str] = set()
    for path in SERVER_SRC.rglob("*"):
        if path.suffix not in (".cpp", ".h"):
            continue
        text = path.read_text(encoding="utf-8", errors="replace")
        for m in LOCALIZE_RE.finditer(text):
            keys.add(unescape_c_string(m.group(1)))
    return keys


def scan_extra_lstr_keys() -> set[str]:
    """Scrap/store names used as lstr keys without _()."""
    extra: set[str] = set()
    scrap_info = SERVER_SRC / "scrap" / "scrap_info.cpp"
    if scrap_info.is_file():
        text = scrap_info.read_text(encoding="utf-8", errors="replace")
        extra.update(REGISTER_SCRAP_RE.findall(text))
    actions = SERVER_SRC / "lc" / "ui" / "terminal_actions.cpp"
    if actions.is_file():
        text = actions.read_text(encoding="utf-8", errors="replace")
        extra.update(STORE_NAME_RE.findall(text))
    return extra


def load_fix_en_translations() -> dict[str, str]:
    if not FIX_EN.is_file():
        return {}
    text = FIX_EN.read_text(encoding="utf-8")
    start = text.index("TRANSLATIONS = {")
    end = text.index("\ndef main", start)
    return ast.literal_eval(text[start + len("TRANSLATIONS = ") : end])


def load_json_map(path: Path) -> dict[str, str]:
    if not path.is_file():
        return {}
    with path.open(encoding="utf-8") as f:
        data = json.load(f)
    if isinstance(data, dict) and "translation" in data:
        return {e["key"]: e["value"] for e in data["translation"]}
    return dict(data)


def load_existing_en() -> dict[str, str]:
    if not EN_JSON.is_file():
        return {}
    with EN_JSON.open(encoding="utf-8") as f:
        data = json.load(f)
    return {e["key"]: e["value"] for e in data.get("translation", [])}


def needs_translation(key: str, value: str | None) -> bool:
    if value is None:
        return True
    if value != key:
        return False
    return bool(CHINESE_RE.search(key))


def build_en_map() -> dict[str, str]:
    en_map: dict[str, str] = {}
    en_map.update(load_fix_en_translations())
    en_map.update(load_existing_en())
    en_map.update(load_json_map(EN_MAP_JSON))
    try:
        from build_full_en_map import get_full_en_map

        en_map.update(get_full_en_map())
    except ImportError:
        pass
    return en_map


def write_translation_json(path: Path, entries: list[dict[str, str]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        json.dump({"translation": entries}, f, ensure_ascii=False, indent=4)
        f.write("\n")


def sort_entries(entries: list[dict[str, str]]) -> list[dict[str, str]]:
    same = [e for e in entries if e["key"] == e["value"]]
    diff = [e for e in entries if e["key"] != e["value"]]
    same.sort(key=lambda e: e["key"])
    diff.sort(key=lambda e: e["key"])
    return same + diff


def main() -> int:
    keys = scan_localize_keys() | scan_extra_lstr_keys()
    en_map = build_en_map()

    prior = load_existing_en()
    entries: list[dict[str, str]] = []
    newly_translated = 0
    missing: list[str] = []

    for key in sorted(keys):
        prev_map = en_map.get(key)
        prev_en = prior.get(key)
        if needs_translation(key, prev_map):
            if key in en_map and en_map[key] != key:
                value = en_map[key]
            else:
                missing.append(key)
                value = key
        else:
            value = prev_map if prev_map is not None else key

        if needs_translation(key, prev_en) and value != key:
            newly_translated += 1

        entries.append({"key": key, "value": value})

    write_translation_json(EN_JSON, sort_entries(entries))
    write_translation_json(CN_JSON, [])

    translated = sum(1 for e in entries if e["key"] != e["value"])
    print(f"Script: {Path(__file__).resolve()}")
    print(f"Keys scanned: {len(keys)}")
    print(f"Translated entries (key != value): {translated}")
    print(f"Newly translated vs prior en.json: {newly_translated}")

    if missing:
        print(f"WARNING: {len(missing)} keys still lack English in en_map.json", file=sys.stderr)
        for k in missing[:20]:
            print(f"  - {k!r}", file=sys.stderr)
        if len(missing) > 20:
            print(f"  ... and {len(missing) - 20} more", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
