# Lethal Company (Teeworlds Mod)

**LeTeehal Company** is a **Lethal Company**-style cooperative scrap-run mod for **Teeworlds 0.6.4**.  
Connect with the vanilla Teeworlds 0.6 client to a `Leteehal-Server` instance.

- Mod version: **0.4.5**
- GameType: `LeteehalCompany`
- Authors: Flower, ST-Chara
- Upstream: https://github.com/NewTeeworldsCN/Teeworlds-LeTeehal

## Gameplay loop (like LC)

1. **Company ship** (`lc_main`) — press **ESC** to vote: check **QUOTA**, pick a **MOON**, buy from **STORE**, vote **LAUNCH**. Press **F3** for the interactive terminal (store, guides, team status).
2. **Facility expedition** — procedural indoor map, collect **scrap** with hammer (4-slot inventory, weight slows you)
3. **Return** — deposit scrap in the **landing ship**, vote **LAUNCH** to leave early, or beat the **SHIFT** timer
4. **Settlement** — scrap value adds to company **credits**; abandoned employees are penalized; MVP and cycle stats shown in chat
5. **Deadline** — meet **QUOTA** before **DEADLINE** days run out, or get **TERMINATED** (reset)

Press **ESC** for the vote menu; press **F3** for the terminal menu (scroll with mouse wheel, hook to go back).

**Tip:** System messages (welcome, settlement, mid-join) use **chat**, not MOTD, so they no longer overwrite the F3 terminal.

### Chat commands

| Command | Description |
|---------|-------------|
| `/help` | LC gameplay guide |
| `/help store` | Company store item guide |
| `/help monsters` | Monster bestiary (with countermeasures in F3) |
| `/help scrap` | Scrap item guide |
| `/status` | Quota, deadline, moon, shift timer |
| `/status me` | Personal career + current round stats |
| `/about` | Mod info |
| `/language en` | Switch language |

### Rcon / server console

| Command | Description |
|---------|-------------|
| `gc_status` | Quota, money, days, phase, seed |
| `gc_set_quota <n>` | Set quota target |
| `gc_set_money <n>` | Set company credits |
| `mapgen_now` | Queue map generation (lobby only) |
| `spawn_monster [type]` | Spawn monster type 0–8 |
| `reload` | Reload current map |

## Moons (routes)

Select in ship terminal before launch (`gc_moon` 0–4). Vote menu shows hazard stars, shift length, and scrap multiplier per route.

| Moon | Hazard | Facility | Theme | Shift | Scrap | Monsters |
|------|--------|----------|-------|-------|-------|----------|
| **Experimentation** 实验 | ★ | **Warehouse** (grid halls) | metal | 7 min | 75% | 65% |
| **Assurance** 保障 | ★★ | **Mansion** (wide halls, grass) | grass | 5 min | 100% | 100% |
| **Vow** 誓约 | ★★★ | **Research** (grid labs) | jungle | 4 min | 130% | 130% |
| **Offense** 攻势 | ★★★★ | **Mines** (tight tunnels, gas/mines) | desert | 3 min | 160% | 160% |
| **Titan** 泰坦 | ★★★★★ | **Mines** (extreme) | winter | 2 min | 180% | 180% |

Themes (`sv_mapgen_theme`): `metal_main`, `grass_main`, `desert_main`, `jungle_main`, `winter_main`. Vote: `theme metal|grass|desert|jungle|winter`.

Each moon uses a different map theme and **facility layout** (grid vs organic rooms, corridor width, hazard mix).

## Company store (lobby terminal)

Spend **credits** (`gc_money`) before launch. Vote and F3 terminal show budget and route-based recommendations.

| Item | Cost | Effect |
|------|------|--------|
| Extended shift | 150 | +2 minutes next expedition |
| Flashbang kit | 80 | Stop sign scrap next expedition |
| Hazard suit | 100 | +3 armor next expedition |
| Life support | 85 | +4 health next expedition |
| Shotgun | 120 | 8 shells next expedition |
| Laser rifle | 140 | 5 shots next expedition |
| Grenade | 160 | 3 grenades next expedition |
| Pistol | 95 | 12 shots next expedition |
| Ninja | 110 | Ninja sword next expedition |
| Medkit | 90 | Medkit scrap next expedition |
| Whistle | 70 | Whistle scrap next expedition |
| Soda | 60 | Energy soda next expedition |
| Megaphone | 110 | Megaphone scrap (scan monsters) |
| Boombox | 175 | Boombox scrap (stun) |
| Remote | 125 | Remote scrap (spawn monster) |
| Aircraft | 220 | Deployable flyer stock |

## Monsters (LC-inspired)

| Type | Name | Behavior |
|------|------|----------|
| 0 | 布条怪 | Hook + weight |
| 1 | 囤积虫 | Chases scrap |
| 2 | 孢子蜥 | Infection debuff |
| 3 | 蔓背怪 (Bracken) | Hides in **grass**, ambush + faster hook |
| 4 | 弹簧头 | **Coil-Head**: flee + stare-freeze if you look at it |
| 5 | 猛禽 | Ranged hunter, fires pistol shots |
| 6 | 爆壳虫 | Slow chaser, explodes on contact or death |
| 7 | 吸盘怪 | Hook + drains health when close |
| 8 | 潜追者 | Fast ground chaser, low HP |

Open **F3 → 图鉴 → monster detail** for three-line countermeasures per type.

Boss every **5 rounds** (typed boss, extra company bonus). Rcon: `spawn_monster 4`.

## Indoor hazards

Procedural hazards scale with moon **hazard stars** and **facility type**:

| Hazard | Effect | Visual |
|--------|--------|--------|
| **Gas** | Periodic damage + on-screen warning | Green doodads + laser frame |
| **Landmine** | One-shot trigger on step, then cleared | Marker tiles + laser cross |
| **Grass** | Bracken ambush zones (monster hidden until you approach) | Grass doodads |
| **Shock** | Electric floor — periodic damage | Yellow warning tiles + lasers |
| **Spike** | Damage when stepping on tile | Spike foreground + lasers |
| **Tar** | Slow movement | Dark tar tiles |

**Fixed rooms** (1–3 per expedition, scan-able): **Battery room** (shock floor), **Fuse room** (mines), **Generator room** (gas leak).

## Scrap

LC-style items: bottle cap, stop sign, gold bar, cash register, bolt, etc.  
**4 inventory slots**. Vote reason `1` to use consumables (flashbang stuns monsters, etc.).

## Death & revive

- Suit damage = freeze in place, drop backpack scrap
- Enter landing ship = auto-repair; teammate **hammer** = revive
- Disconnect during expedition = 2 min persist (same round)
- Mid-expedition join = spawn at landing ship + briefing chat (timer, moon, map seed)

## Progress & achievements

- Cycle stats and per-player career saved to `leteehal_company_stats.txt` (survives server restart)
- Achievements: first boss kill, zero-abandon return, quota met (broadcast once per player)
- Settlement shows MVP (top deposit / revives) and ASCII quota progress bar in chat

## Build

From project root (recommended):

```bash
make server
```

Or manually:

```bash
mkdir -p build && cd build
cmake ..
make -j$(nproc) Leteehal-Server
```

After adding or renaming `.cpp` files, run `cmake ..` again in `build/` so CMake picks them up.

Binary: `build/Leteehal-Server`

### Localization check

```bash
python3 scripts/generate_server_lang.py
python3 scripts/check_server_lang.py
```

### Test checklist

1. Lobby: F3 terminal scroll + ESC vote menu both work; chat messages do not wipe F3 menu
2. `/language en` then `/language cn` — chat, vote, terminal text follow language
3. Vote LAUNCH → map generation shows stage hints; failure message if layout fails
4. In facility: proximity warnings by monster type, scan fixed rooms, store purchases, aircraft deploy
5. Return: settlement chat, MVP, `/status me`, career file updated

## Run

```bash
./build/Leteehal-Server -f server.cfg.example
```

### Important cvars

| Cvar | Default | Description |
|------|---------|-------------|
| `gc_quota` | 280 | Profit quota target (scales per round) |
| `gc_money` | 0 | Company credits |
| `gc_days` | 4 | Days until deadline |
| `gc_moon` | 1 | Selected moon (0–4) |
| `sv_timelimit` | 5 | Shift minutes (overridden by moon) |
| `sv_less_players_start` | 2 | Min employees to launch |
| `sv_mapgen_theme` | metal_main | Automapper rules |
| `sv_mapgen` | 1 | Procedural facilities |
| `sv_mapgen_random_seed` | 1 | Random seed each expedition |

Balance formulas: `src/game/server/lc/expedition/balance.h`  
Moon config: `src/game/server/lc/expedition/moons.cpp`

## License

Based on Teeworlds 0.6.4. See `license.txt`.
