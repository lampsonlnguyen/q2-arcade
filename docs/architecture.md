# Q2 Arcade architecture and migration

## Scope

Q2 Arcade packages the three games **newly introduced on `q2-pod/new-builtin-games`** (Pong, Blocks, Breakout) into one downloadable Q2 Pod card app. The four older games (Sudoku, Vortex, Solitaire and Snake) are already part of Q2 Pod, so their firmware implementations are **not copied** into this optional app.

This repository **does not** merge or remove firmware changes on the original `new-builtin-games` branch. Firmware-specific code such as `arcade_draw.inc`, `arcade_gameplay.inc`, `games.c`, and `peq.h` references internal firmware APIs and is replaced here by the card's own runtime, menu, drawing and inputs.

## Source-to-card migration

| Original firmware branch | Standalone card app |
| --- | --- |
| `patch/arcade_core.c/.h` | `core/arcade_core.c/.h` |
| `patch/breakout_core.c/.h` | `core/breakout_core.c/.h` |
| `patch/game_framework.c/.h` | `core/game_framework.c/.h` |
| `patch/arcade_gameplay.inc` | `src/app.c` |
| `patch/arcade_draw.inc` | `src/render.c` |
| Games list and timers inside `patch/games.c` | `src/app.c` and `src/platform_q2.c` |
| Firmware blob persistence | `.arcade/highscores` on SD |
| Firmware builder `tools/peq.py` | Standalone `Makefile` and ZIP |

The portable game modules retain their original state layout and algorithms; they use integer-only physics, static state and no dynamic allocation. Card input is received via the Q2 input datagram protocol. The `q2app` manifest must declare `audio=1`: without it, Q2 Pod's `addon_game_input()` reserves the physical Left, Right, and Play/Pause keys for music rather than forwarding them to the card app. This also stops background music while Arcade is active; Arcade itself does not currently produce sound. Blocks wheel rotation uses a quarter-turn threshold (approximately 50 raw units) and caps each wheel packet at one rotation. The framebuffer is the same rotated 375×320 display exposed to other Q2 card programs.

## Q2 Pod theme-aware game selection

`src/theme.c/.h` reads the device's existing `[IPOD]` preferences
from `/mnt/data/config.ini` (read-only). The settings match the
[upstream theme XML](https://github.com/DiamondBond/q2-pod/tree/main/assets/themes)
and `patch/theme_engine.c`:

- `THEME=0`: Minimal, using `MINIMAL_ACCENT` and a framed selection card
- `THEME=1`: Classic, using `CLASSIC_LIGHT_ACCENT` and a gloss selection band
- `THEME=2`: Modern, using `ACCENT` and a flat selection bar

The numeric accent palette follows upstream `patch/offsets.inc`. The
separate Q2 card process cannot invoke AWTK theme widgets; `src/render.c`
therefore paints compact native-framebuffer versions of the selection
style, retaining the existing input hitboxes and using the shared
rounded-display text safe area.

The selected theme is loaded on app launch, not continuously. `LIGHT=1`
uses a bright menu surface for Minimal/Modern; `LIGHT=2` (Auto) uses the
dark fallback because the firmware's live day/night decision is not
published to card apps. Gameplay surfaces do not change with the theme.

## Shared rounded-screen safe area

The Q2 display is physically rounded, even though the framebuffer is 375×320.
All **HUD elements, captions, labels, and interactive UI text** should pass
through `q2_game_ui_safe_point(width, height, x, y)` from
`core/game_framework.h` before rendering. The shared `src/render.c` text
primitive now clips individual glyph pixels to that safe region, so new games
using it inherit the policy automatically.

The region excludes a narrow band at the very top and bottom, uses a
conservative horizontal margin across the middle, and reserves wider
horizontal corner margins within the upper/lower eighth of the display.
The layout is proportional to screen size rather than hardcoded to Blocks.

**Do not clip game physics or the full framebuffer to the text-safe area.**
Backgrounds, borders and the playfield can still occupy the available screen.
Keep game-important UI text and touch targets inside the shared inset, rather
than placing labels in the four corners. For a new text renderer, call the
framework helper rather than duplicating its constants.

Blocks' Score/score value has moved right; the bottom instruction line is
also inset from the lower-left rounded corner.

### Adding a fourth arcade game

Implement a new portable core in `core/`, add its launcher registration in `src/app.c`, and draw it in `src/render.c`. This avoids modifying Q2 Pod firmware, but the current card still requires some per-game integration. The separate `game_framework` module is available for a later callback/registry refactor.

### Testing

`make check` runs native C tests without Q2 hardware. `make` cross-compiles a MIPS32 ELF and creates an installable ZIP. GitHub Actions performs both. A physical Shanling Q2 remains necessary to verify wheel direction, touch, frame-rate and persistent scores.

### Card app limitations

A card app is a userspace program and cannot simply reuse firmware-specific AWTK windows or internal `games.c` hooks. Any future migration of the **old four built-in games** would be an additional port rather than a straight file copy. This is a collection of three games, not an app-store service or a dynamic downloader for arbitrary third-party game code.
