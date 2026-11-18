# Q2 Arcade — downloadable game collection for Shanling Q2

Q2 Arcade brings **Pong**, **Blocks (Tetris-style)**, **Breakout**, **Tic-Tac-Toe**, **Connect Four**, **Battleship**, **Road Dodge**, and **Catch** to Q2 Pod as **one optional SD-card application**. No firmware patch or replacement of Q2 Pod's existing built-in games is needed.

The original Pong/Blocks/Breakout gameplay cores and the portable framework are migrated from [`q2-pod/new-builtin-games`](https://github.com/lampsonlnguyen/q2-pod/tree/new-builtin-games). Unlike the original firmware version, these games are self-contained in one MIPS card executable with a launcher, Q2 framebuffer renderer and input-socket client.

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

The launcher now contains eight games. Turn the wheel to scroll the list and
press Centre to open the highlighted game. Four entries are visible at once,
with the list scrolling automatically. Touch selection is also supported.

| Game | Controls | Goal |
| --- | --- | --- |
| Pong | Wheel or Left/Right moves paddle | Outscore the CPU |
| Blocks | Wheel rotates, Left/Right moves, Play soft-drops, Centre hard-drops | Clear rows |
| Breakout | Wheel or Left/Right moves paddle, Centre launches | Break all bricks |
| Tic-Tac-Toe | Choose AI or local two-player and 3×3, 6×6, or 9×9; wheel selects, Centre places, Play moves down a row, or tap | Connect 3, 4, or 5 marks respectively |
| Connect Four | Wheel selects a column, Centre drops a disc, or tap a column | Align four against CPU |
| Battleship | Wheel selects a cell, Centre fires, or tap a cell | Sink four hidden ships (eight segments) in 26 shots |
| Road Dodge | Wheel, Left/Right, or tap to change lanes | Steer your car around oncoming traffic |
| Catch | Wheel, Left/Right, or tap to change lanes | Catch falling apples in a basket, avoid misses |

Press Return/Back while playing to open the common pause menu. The games
use compact, original C graphics and game rules; no proprietary handheld
LCD game ROMs or artwork are included.

Within the pause menu, use the wheel or physical Left/Right buttons to select **Resume**, **Restart**, **Games**, or **Exit**, then press Centre. Return from the pause menu goes back to the arcade's game list. Touch taps on the game picker are supported.

**Physical button input:** The card manifest sets `audio=1` so Q2 Pod forwards the Left, Right, and Play/Pause hardware buttons to Arcade rather than the music player. This also stops music playback while Arcade is open. Despite the flag name, Arcade does not generate audio. The wheel rotates Blocks one step per roughly quarter turn, with large flicks limited to one rotation per input packet.

High scores for Blocks and Breakout are saved to `.arcade/highscores` on the SD card.

## Matching the Q2 Pod display theme

The **Arcade game-selection screen** reads the active theme from Q2 Pod's
`/mnt/data/config.ini` at launch. The same numeric Theme IDs and per-theme
Accent settings from [DiamondBond's theme sources](https://github.com/DiamondBond/q2-pod/tree/main/assets/themes)
are used:

- **Minimal:** themed background and accent-outlined selection cards.
- **Modern:** accent-colored selection bars and flat list layout.
- **Classic:** light iPod-inspired layout with a glossy accent selection band.

`LIGHT=1` enables a light surface for Minimal/Modern; `LIGHT=2` (Auto)
uses the light/dark appearance of the firmware screen at Arcade launch. The theme is captured when Arcade
starts; reopen the app after changing Q2 Pod's theme. Gameplay graphics
remain independent from the theme.

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
- `core/mini_games.c/.h` — Tic-Tac-Toe, Connect Four, Battleship, Road Dodge and Catch rules.
- `core/game_framework.c/.h` — shared lifecycle, input, update callback infrastructure.
- `src/app.c/.h` — arcade launcher and input/update adapters.
- `src/render.c` — lightweight frame drawing without AWTK dependencies.
- `src/platform_q2.c` — MIPS Linux framebuffer, input datagram socket, timekeeping and SD-card persistence.
- `tests/` — portable rules, framework and application-input regression tests.

The framework module is included for continued development, but the current games use portable rules with a shared card-level adapter; they have not yet been migrated to the framework callback runner.

For migration notes, implementation boundaries and contributor guidance, see [docs/architecture.md](docs/architecture.md). For the open-source algorithm references and new rendering details, see [docs/mini-games.md](docs/mini-games.md).

## Credits and license

Portable games and framework were developed for the `new-builtin-games` branch of Q2 Pod. Q2 card startup, input and frame-buffer integration follow the approach used in [DiamondBond's Q2 Fidget/Buddy](https://github.com/DiamondBond/q2-fidget), [Q2 2048](https://github.com/lampsonlnguyen/q2-2048) and [Q2 Dino Runner](https://github.com/lampsonlnguyen/q2-dino-runner). Original source remains GPL-3.0-or-later; see [LICENSE](LICENSE). No stock firmware or proprietary game binaries are distributed.

### Appearance

Arcade reads `/mnt/data/config.ini` when opened and follows Q2 Pod’s Minimal, Modern, or Classic theme, its saved accent, and Light Mode. Auto Light Mode inherits the visible firmware screen’s light/dark appearance at launch; reopen Arcade after a theme change or a day/night transition. Classic uses a flat selection band rather than the firmware’s gloss artwork. Game pieces keep distinct colors for readability. Bottom hints use compact 3×5 lettering centered inside the rounded screen.

Menu rows are compact (38-pixel spacing) to leave room for additional entries. Interface text uses smooth glyphs close to the original size, with centered headings and safe screen margins.

## Automatic builds and releases

GitHub Actions tests and builds each push to `main` and every pull request. Download the **q2-arcade-card-app** artifact from a successful run, or start a build with **Actions → q2-arcade → Run workflow**.

Push an unused version tag to publish a release automatically:

```sh
git pull --ff-only
git tag v1.0.1
git push origin v1.0.1
```

Once the build and package checks pass, Actions creates the release and attaches **q2-arcade.zip** directly. Re-running the tag build replaces that release's ZIP. Branch pushes and manual branch builds only save Actions artifacts.

### Tic-Tac-Toe setup

Opening Tic-Tac-Toe shows **Player vs AI**, **Board**, and **Start Game**. Select a setting and press Centre (or tap it) to cycle. Two-player mode shares the Q2: Player 1 is X, Player 2 is O, and the turn is shown above the board. Restart keeps your chosen mode and size. Return from setup goes back to the game list.

3×3 requires 3 in a row, 6×6 requires 4, and 9×9 requires 5, horizontally, vertically, or diagonally. The 3×3 AI uses minimax; larger boards use bounded open-line scoring with immediate win/block detection. X and O are original transparent PNG sprites in `assets/sprites`, packed into `src/ttt_sprites.h`; no runtime image decoder is needed. To regenerate them, install Pillow and run `python3 tools/make_ttt_sprites.py`.
