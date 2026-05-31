#!/usr/bin/env python3
"""Move game/server sources into subdirectories and update include paths."""

from __future__ import annotations

import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SERVER = ROOT / "src" / "game" / "server"

MOVES: list[tuple[str, str]] = [
    # core
    ("gamecontext.cpp", "core/gamecontext.cpp"),
    ("gamecontext.h", "core/gamecontext.h"),
    ("gamecontroller.cpp", "core/gamecontroller.cpp"),
    ("gamecontroller.h", "core/gamecontroller.h"),
    ("player.cpp", "core/player.cpp"),
    ("player.h", "core/player.h"),
    ("gameworld.cpp", "core/gameworld.cpp"),
    ("gameworld.h", "core/gameworld.h"),
    ("entity.cpp", "core/entity.cpp"),
    ("entity.h", "core/entity.h"),
    ("eventhandler.cpp", "core/eventhandler.cpp"),
    ("eventhandler.h", "core/eventhandler.h"),
    ("alloc.h", "core/alloc.h"),
    # lc/expedition
    ("lc_expedition.h", "lc/expedition/expedition.h"),
    ("lc_moons.cpp", "lc/expedition/moons.cpp"),
    ("lc_moons.h", "lc/expedition/moons.h"),
    ("game_balance.h", "lc/expedition/balance.h"),
    # lc/economy
    ("lc_store_bonus.h", "lc/economy/store_bonus.h"),
    ("lc_player_persist.h", "lc/economy/player_persist.h"),
    # lc/hazards
    ("lc_hazards.cpp", "lc/hazards/hazards.cpp"),
    ("lc_hazards.h", "lc/hazards/hazards.h"),
    # lc/ui
    ("lc_guide.cpp", "lc/ui/guide.cpp"),
    ("lc_guide.h", "lc/ui/guide.h"),
    ("lc_terminal_menu.cpp", "lc/ui/terminal_menu.cpp"),
    ("lc_terminal_menu.h", "lc/ui/terminal_menu.h"),
    ("lc_terminal_menu_nav.cpp", "lc/ui/terminal_menu_nav.cpp"),
    ("lc_terminal_actions.cpp", "lc/ui/terminal_actions.cpp"),
    ("lc_terminal_actions.h", "lc/ui/terminal_actions.h"),
    ("lc_vote_menu.cpp", "lc/ui/vote_menu.cpp"),
    ("lc_vote_menu.h", "lc/ui/vote_menu.h"),
    ("lc_localize_util.cpp", "lc/ui/localize_util.cpp"),
    ("lc_localize_util.h", "lc/ui/localize_util.h"),
    # lc/mapgen
    ("mapgen.cpp", "lc/mapgen/mapgen.cpp"),
    ("mapgen.h", "lc/mapgen/mapgen.h"),
    ("mapgen/maze.cpp", "lc/mapgen/maze.cpp"),
    ("mapgen/maze.h", "lc/mapgen/maze.h"),
    ("mapgen/room.cpp", "lc/mapgen/room.cpp"),
    ("mapgen/room.h", "lc/mapgen/room.h"),
    ("mapgen/gen_layer.cpp", "lc/mapgen/gen_layer.cpp"),
    ("mapgen/gen_layer.h", "lc/mapgen/gen_layer.h"),
    ("mapgen/mapgen_random.cpp", "lc/mapgen/mapgen_random.cpp"),
    ("mapgen/mapgen_random.h", "lc/mapgen/mapgen_random.h"),
    # entities/core
    ("entities/character.cpp", "entities/core/character.cpp"),
    ("entities/character.h", "entities/core/character.h"),
    ("entities/projectile.cpp", "entities/core/projectile.cpp"),
    ("entities/projectile.h", "entities/core/projectile.h"),
    ("entities/laser.cpp", "entities/core/laser.cpp"),
    ("entities/laser.h", "entities/core/laser.h"),
    ("entities/pickup.cpp", "entities/core/pickup.cpp"),
    ("entities/pickup.h", "entities/core/pickup.h"),
    ("entities/flag.cpp", "entities/core/flag.cpp"),
    ("entities/flag.h", "entities/core/flag.h"),
    # entities/lc
    ("entities/monster.cpp", "entities/lc/monster.cpp"),
    ("entities/monster.h", "entities/lc/monster.h"),
    ("entities/scrap.cpp", "entities/lc/scrap.cpp"),
    ("entities/scrap.h", "entities/lc/scrap.h"),
    ("entities/ship.cpp", "entities/lc/ship.cpp"),
    ("entities/ship.h", "entities/lc/ship.h"),
    ("entities/scan_link.cpp", "entities/lc/scan_link.cpp"),
    ("entities/scan_link.h", "entities/lc/scan_link.h"),
    ("entities/hazard_marker.cpp", "entities/lc/hazard_marker.cpp"),
    ("entities/hazard_marker.h", "entities/lc/hazard_marker.h"),
    # scrap
    ("scrap-data.cpp", "scrap/scrap_data.cpp"),
    ("scrap-info.cpp", "scrap/scrap_info.cpp"),
    ("scrap-info.h", "scrap/scrap_info.h"),
]

# Longest matches first.
INCLUDE_REPLACEMENTS: list[tuple[str, str]] = [
    # mapgen
    ("<game/server/mapgen/mapgen_random.h>", "<game/server/lc/mapgen/mapgen_random.h>"),
    ("<game/server/mapgen/gen_layer.h>", "<game/server/lc/mapgen/gen_layer.h>"),
    ("<game/server/mapgen/room.h>", "<game/server/lc/mapgen/room.h>"),
    ("<game/server/mapgen/maze.h>", "<game/server/lc/mapgen/maze.h>"),
    ("<game/server/mapgen.h>", "<game/server/lc/mapgen/mapgen.h>"),
    # entities
    ("<game/server/entities/character.h>", "<game/server/entities/core/character.h>"),
    ("<game/server/entities/projectile.h>", "<game/server/entities/core/projectile.h>"),
    ("<game/server/entities/laser.h>", "<game/server/entities/core/laser.h>"),
    ("<game/server/entities/pickup.h>", "<game/server/entities/core/pickup.h>"),
    ("<game/server/entities/flag.h>", "<game/server/entities/core/flag.h>"),
    ("<game/server/entities/monster.h>", "<game/server/entities/lc/monster.h>"),
    ("<game/server/entities/scrap.h>", "<game/server/entities/lc/scrap.h>"),
    ("<game/server/entities/ship.h>", "<game/server/entities/lc/ship.h>"),
    ("<game/server/entities/scan_link.h>", "<game/server/entities/lc/scan_link.h>"),
    ("<game/server/entities/hazard_marker.h>", "<game/server/entities/lc/hazard_marker.h>"),
    # scrap
    ("<game/server/scrap-info.h>", "<game/server/scrap/scrap_info.h>"),
    # lc ui
    ("<game/server/lc_terminal_menu.h>", "<game/server/lc/ui/terminal_menu.h>"),
    ("<game/server/lc_terminal_actions.h>", "<game/server/lc/ui/terminal_actions.h>"),
    ("<game/server/lc_vote_menu.h>", "<game/server/lc/ui/vote_menu.h>"),
    ("<game/server/lc_guide.h>", "<game/server/lc/ui/guide.h>"),
    ("<game/server/lc_localize_util.h>", "<game/server/lc/ui/localize_util.h>"),
    # lc hazards / expedition
    ("<game/server/lc_hazards.h>", "<game/server/lc/hazards/hazards.h>"),
    ("<game/server/lc_moons.h>", "<game/server/lc/expedition/moons.h>"),
    ("<game/server/game_balance.h>", "<game/server/lc/expedition/balance.h>"),
    ("<game/server/lc_expedition.h>", "<game/server/lc/expedition/expedition.h>"),
    ("<game/server/lc_store_bonus.h>", "<game/server/lc/economy/store_bonus.h>"),
    ("<game/server/lc_player_persist.h>", "<game/server/lc/economy/player_persist.h>"),
    # core
    ("<game/server/gamecontext.h>", "<game/server/core/gamecontext.h>"),
    ("<game/server/gamecontroller.h>", "<game/server/core/gamecontroller.h>"),
    ("<game/server/player.h>", "<game/server/core/player.h>"),
    ("<game/server/gameworld.h>", "<game/server/core/gameworld.h>"),
    ("<game/server/entity.h>", "<game/server/core/entity.h>"),
    ("<game/server/eventhandler.h>", "<game/server/core/eventhandler.h>"),
    ("<game/server/alloc.h>", "<game/server/core/alloc.h>"),
]


def git_mv(src_rel: str, dst_rel: str) -> None:
    src = SERVER / src_rel
    dst = SERVER / dst_rel
    if dst.exists() or not src.exists():
        return
    dst.parent.mkdir(parents=True, exist_ok=True)
    result = subprocess.run(
        ["git", "mv", str(src), str(dst)],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        shutil.move(str(src), str(dst))


def patch_includes() -> None:
    for path in (ROOT / "src").rglob("*"):
        if path.suffix not in {".cpp", ".h", ".c"}:
            continue
        text = path.read_text(encoding="utf-8")
        original = text
        for old, new in INCLUDE_REPLACEMENTS:
            text = text.replace(old, new)
        if text != original:
            path.write_text(text, encoding="utf-8")


def replace_in_file(path: Path, replacements: dict[str, str]) -> None:
    if not path.exists():
        return
    text = path.read_text(encoding="utf-8")
    original = text
    for old, new in replacements.items():
        text = text.replace(old, new)
    if text != original:
        path.write_text(text, encoding="utf-8")


def fix_relative_includes() -> None:
    """Update quoted includes after directory moves."""
    # core/
    replace_in_file(SERVER / "core" / "gamecontext.h", {
        '"scrap-info.h"': '"../scrap/scrap_info.h"',
        '"game_balance.h"': '"../lc/expedition/balance.h"',
        '"mapgen.h"': '"../lc/mapgen/mapgen.h"',
        '"lc_player_persist.h"': '"../lc/economy/player_persist.h"',
        '"lc_store_bonus.h"': '"../lc/economy/store_bonus.h"',
        '"lc_terminal_menu.h"': '"../lc/ui/terminal_menu.h"',
        '"lc_vote_menu.h"': '"../lc/ui/vote_menu.h"',
        '"entities/monster.h"': '"../entities/lc/monster.h"',
    })
    replace_in_file(SERVER / "core" / "gamecontext.cpp", {
        '"entities/scrap.h"': '"../entities/lc/scrap.h"',
        '"entities/ship.h"': '"../entities/lc/ship.h"',
        '"game_balance.h"': '"../lc/expedition/balance.h"',
        '"lc_moons.h"': '"../lc/expedition/moons.h"',
        '"lc_guide.h"': '"../lc/ui/guide.h"',
        '"scrap-info.h"': '"../scrap/scrap_info.h"',
        '"entities/monster.h"': '"../entities/lc/monster.h"',
        '"entities/scan_link.h"': '"../entities/lc/scan_link.h"',
        '"entities/hazard_marker.h"': '"../entities/lc/hazard_marker.h"',
        '"lc_hazards.h"': '"../lc/hazards/hazards.h"',
        '"entities/vehicle/aircraft.h"': '"../entities/vehicle/aircraft.h"',
        '"entities/vehicle/vehicle_util.h"': '"../entities/vehicle/vehicle_util.h"',
    })
    replace_in_file(SERVER / "core" / "gamecontroller.h", {
        '"entities/ship.h"': '"../entities/lc/ship.h"',
        '"lc_expedition.h"': '"../lc/expedition/expedition.h"',
    })
    replace_in_file(SERVER / "core" / "gamecontroller.cpp", {
        '"game_balance.h"': '"../lc/expedition/balance.h"',
        '"lc_moons.h"': '"../lc/expedition/moons.h"',
        '"entities/pickup.h"': '"../entities/core/pickup.h"',
        '"entities/ship.h"': '"../entities/lc/ship.h"',
    })
    replace_in_file(SERVER / "core" / "player.h", {
        '"entities/character.h"': '"../entities/core/character.h"',
        '"scrap-info.h"': '"../scrap/scrap_info.h"',
    })

    # lc/ui/
    for name in ("guide.cpp", "terminal_menu.cpp", "terminal_menu_nav.cpp",
                 "vote_menu.cpp", "terminal_actions.cpp"):
        replace_in_file(SERVER / "lc" / "ui" / name, {
            '"lc_guide.h"': '"guide.h"',
            '"lc_terminal_menu.h"': '"terminal_menu.h"',
            '"lc_terminal_actions.h"': '"terminal_actions.h"',
            '"lc_vote_menu.h"': '"vote_menu.h"',
            '"lc_localize_util.h"': '"localize_util.h"',
            '"game_balance.h"': '"../expedition/balance.h"',
            '"lc_moons.h"': '"../expedition/moons.h"',
            '"gamecontext.h"': '"../../core/gamecontext.h"',
            '"gameworld.h"': '"../../core/gameworld.h"',
            '"player.h"': '"../../core/player.h"',
            '"scrap-info.h"': '"../../scrap/scrap_info.h"',
            '"entities/character.h"': '"../../entities/core/character.h"',
            '"entities/monster.h"': '"../../entities/lc/monster.h"',
            '"entities/scrap.h"': '"../../entities/lc/scrap.h"',
        })

    replace_in_file(SERVER / "lc" / "ui" / "terminal_actions.cpp", {
        '"entities/monster.h"': '"../../entities/lc/monster.h"',
    })

    # lc/hazards/
    replace_in_file(SERVER / "lc" / "hazards" / "hazards.cpp", {
        '"lc_hazards.h"': '"hazards.h"',
    })

    # lc/expedition/
    replace_in_file(SERVER / "lc" / "expedition" / "moons.cpp", {
        '"lc_moons.h"': '"moons.h"',
    })
    replace_in_file(SERVER / "lc" / "expedition" / "moons.h", {
        '"lc_hazards.h"': '"../hazards/hazards.h"',
    })

    # lc/ui headers
    for name in ("guide.h", "terminal_menu.h", "terminal_actions.h",
                 "vote_menu.h", "localize_util.h"):
        replace_in_file(SERVER / "lc" / "ui" / name, {})

    replace_in_file(SERVER / "lc" / "ui" / "guide.cpp", {
        '"lc_guide.h"': '"guide.h"',
        '"game_balance.h"': '"../expedition/balance.h"',
        '"gamecontext.h"': '"../../core/gamecontext.h"',
        '"gameworld.h"': '"../../core/gameworld.h"',
        '"lc_localize_util.h"': '"localize_util.h"',
        '"lc_terminal_actions.h"': '"terminal_actions.h"',
        '"lc_terminal_menu.h"': '"terminal_menu.h"',
        '"player.h"': '"../../core/player.h"',
        '"scrap-info.h"': '"../../scrap/scrap_info.h"',
        '"entities/character.h"': '"../../entities/core/character.h"',
        '"entities/scrap.h"': '"../../entities/lc/scrap.h"',
        '"entities/monster.h"': '"../../entities/lc/monster.h"',
    })

    replace_in_file(SERVER / "lc" / "ui" / "localize_util.cpp", {
        '"lc_localize_util.h"': '"localize_util.h"',
    })

    replace_in_file(SERVER / "lc" / "ui" / "terminal_actions.cpp", {
        '"lc_terminal_actions.h"': '"terminal_actions.h"',
    })

    replace_in_file(SERVER / "lc" / "ui" / "terminal_menu_nav.cpp", {
        '"lc_terminal_menu.h"': '"terminal_menu.h"',
    })

    replace_in_file(SERVER / "lc" / "ui" / "vote_menu.cpp", {
        '"lc_vote_menu.h"': '"vote_menu.h"',
    })

    replace_in_file(SERVER / "scrap" / "scrap_info.cpp", {
        '"scrap-info.h"': '"scrap_info.h"',
    })

    replace_in_file(SERVER / "scrap" / "scrap_data.cpp", {
        '"scrap-info.h"': '"scrap_info.h"',
    })

    # lc/mapgen/
    replace_in_file(SERVER / "lc" / "mapgen" / "mapgen.cpp", {
        '"mapgen.h"': '"mapgen.h"',
    })

    # entities/core/
    replace_in_file(SERVER / "entities" / "core" / "character.cpp", {
        '"monster.h"': '"../lc/monster.h"',
        '"ship.h"': '"../lc/ship.h"',
        '"vehicle/vehicle_util.h"': '"../vehicle/vehicle_util.h"',
    })
    replace_in_file(SERVER / "entities" / "core" / "projectile.cpp", {
        '"monster.h"': '"../lc/monster.h"',
    })
    replace_in_file(SERVER / "entities" / "core" / "laser.cpp", {
        '"monster.h"': '"../lc/monster.h"',
    })

    # entities/lc/
    replace_in_file(SERVER / "entities" / "lc" / "monster.cpp", {
        '"laser.h"': '"../core/laser.h"',
        '"projectile.h"': '"../core/projectile.h"',
        '"character.h"': '"../core/character.h"',
    })

    # scrap/
    replace_in_file(SERVER / "scrap" / "scrap_info.h", {
        '"entities/scrap.h"': '"../entities/lc/scrap.h"',
        '"gamecontext.h"': '"../core/gamecontext.h"',
    })
    replace_in_file(SERVER / "scrap" / "scrap_info.cpp", {
        '"lc_guide.h"': '"../lc/ui/guide.h"',
    })
    replace_in_file(SERVER / "scrap" / "scrap_data.cpp", {
        '"game_balance.h"': '"../lc/expedition/balance.h"',
        '"entities/monster.h"': '"../entities/lc/monster.h"',
    })

    # lc/economy/
    replace_in_file(SERVER / "lc" / "economy" / "player_persist.h", {
        '"scrap-info.h"': '"../../scrap/scrap_info.h"',
    })


def main() -> None:
    for src, dst in MOVES:
        git_mv(src, dst)
    patch_includes()
    fix_relative_includes()
    print("restructure complete")


if __name__ == "__main__":
    main()
