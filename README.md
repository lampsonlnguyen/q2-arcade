# Q2 Arcade — downloadable game collection for Shanling Q2

Q2 Arcade brings **Pong**, **Blocks (Tetris-style)** and **Breakout** to Q2 Pod as **one optional SD-card application**. No firmware patch or replacement of Q2 Pod's existing built-in games is needed.

The gameplay cores and the portable framework are migrated from [`q2-pod/new-builtin-games`](https://github.com/lampsonlnguyen/q2-pod/tree/new-builtin-games). Unlike the original firmware version, these games are self-contained in one MIPS card executable with a launcher, Q2 framebuffer renderer and input-socket client.

## Install

1. From [the successful GitHub Actions build](https://github.com/lampsonlnguyen/q2-arcade/actions), download the artifact called **`q2-arcade-card-app`**. GitHub gives you a ZIP containing **another ZIP**, `q2-arcade.zip`.
2. **Extract the first ZIP**, then **extract `q2-arcade.zip` directly to the microSD card root**. Do **not** copy just the inner ZIP or leave an extra directory such as `q2-arcade/` around the hidden app folder.
3. Verify these paths exist on the microSD card (replace `E:` with your card's drive letter):
   - `E:\.arcade\q2app` (plain text containing `name=Q2 Arcade` and `game=1`)
   - `E:\.arcade\q2-arcade` (the MIPS executable, not a `.zip` file)
4. Safely eject the SD card, insert it in the Q2, then **leave and reopen Library → Games → Q2 Arcade** so Q2 Pod scans the card again.

**Windows tip:** In File Explorer choose **View → Show → Hidden items** if `.arcade` is not visible.

**If Q2 Arcade still does not appear:** Confirm you are running Q2 Pod firmware with SD-card games support (stock Shanling firmware does not provide this custom Games discovery). The firmware scans hidden folders directly beneath the card root, requires a `q2app` file containing `game=1`, and only lists an app if its `q2-arcade` binary is present. If you installed other card games, verify the Arcade folder sits alongside them, not inside their folders.

The package contains `.arcade/q2app` (`name=Q2 Arcade`, `game=1`) and `.arcade/q2-arcade`, a freestanding MIPS32 little-endian program.

If you installed an older build, rename the SD-card folder `.q2-arcade` to `.arcade`; keep `q2-arcade` and `q2app` inside it. This also preserves `highscores`. The folder ID is `arcade`, so Q2 Pod looks for `.arcade/q2-arcade` and sends input to `/tmp/q2-arcade.input.sock`.

### Controls

| Q2 control | Arcade menu | Pong | Blocks | Breakout |
| --- | --- | --- | --- | --- |
| Click wheel | Select game | Move paddle | Rotate left/right | Move paddle |
| Left / right physical buttons | Change selection | Nudge paddle | Move piece horizontally | Nudge paddle |
| Down / Play-Pause button | — | Pause | Soft drop (up to three rows) | Pause |
| Centre | Launch game | — | Hard drop | Launch ball |
| Return / Back | Exit app | Pause / quit menu | Pause / quit menu | Pause / quit menu |

Within the pause menu, use the wheel or physical Left/Right buttons to select **Resume**, **Restart**, **Games**, or **Exit**, then press Centre. Return from the pause menu goes back to the arcade's game list. Touch taps on the game picker are supported.

High scores for Blocks and Breakout are saved to `.arcade/highscores` on the SD card.

## Build from source

On Ubuntu or WSL, install `clang`, `lld`, `make`, `zip`, and a native C compiler. Then:

```sh
make check
make
```

The distributable file is **`dist/q2-arcade.zip`**. There is no requirement for stock Shanling firmware or a full firmware build. Build validation runs in GitHub Actions; physical Q2 input and framebuffer testing is still required before release.

## Design

- `core/arcade_core.c/.h` — portable Pong and Blocks logic.
- `core/breakout_core.c/.h` — portable Breakout logic.
- `core/game_framework.c/.h` — shared lifecycle, input, update callback infrastructure.
- `src/app.c/.h` — arcade launcher and input/update adapters.
- `src/render.c` — lightweight frame drawing without AWTK dependencies.
- `src/platform_q2.c` — MIPS Linux framebuffer, input datagram socket, timekeeping and SD-card persistence.
- `tests/` — portable rules, framework and application-input regression tests.

The framework module is included for continued development, but the three games currently use the existing portable rules with a shared card-level adapter; they have not yet been migrated to the framework callback runner.

For migration notes, implementation boundaries and contributor guidance, see [docs/architecture.md](docs/architecture.md).

## Credits and license

Portable games and framework were developed for the `new-builtin-games` branch of Q2 Pod. Q2 card startup, input and frame-buffer integration follow the approach used in [DiamondBond's Q2 Fidget/Buddy](https://github.com/DiamondBond/q2-fidget), [Q2 2048](https://github.com/lampsonlnguyen/q2-2048) and [Q2 Dino Runner](https://github.com/lampsonlnguyen/q2-dino-runner). Original source remains GPL-3.0-or-later; see [LICENSE](LICENSE). No stock firmware or proprietary game binaries are distributed.
