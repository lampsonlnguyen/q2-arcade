# Q2 Arcade — downloadable game collection for Shanling Q2

Q2 Arcade brings **Sudoku**, **Bricks**, **Solitaire**, **Snake**, **Pong**, **Tetris**, **Breakout**, **Battleship**, and **Minesweeper** to Q2 Pod as one optional SD-card application.

The original Pong/Blocks/Breakout gameplay cores and the portable framework are migrated from [`q2-pod/new-builtin-games`](https://github.com/lampsonlnguyen/q2-pod/tree/new-builtin-games). Unlike the original firmware version, these games are self-contained in one MIPS card executable with a launcher, Q2 framebuffer renderer and input-socket client.

## Install

1. From [the successful GitHub Actions build](https://github.com/lampsonlnguyen/q2-arcade/actions), download the artifact called **`q2-arcade-card-app`**. GitHub gives you a ZIP containing **another ZIP**, `q2-arcade.zip`.
2. **Extract the first ZIP**, then **extract `q2-arcade.zip` directly to the microSD card root**. Do **not** copy just the inner ZIP or leave an extra directory such as `q2-arcade/` around the hidden app folder.
3. Verify these paths exist on the microSD card (replace `E:` with your card's drive letter):
   - `E:\.arcade\q2app` (plain text containing `name=Arcade` and `row=games`)
   - `E:\.arcade\q2-arcade` (the MIPS executable, not a `.zip` file)
4. Safely eject the SD card, insert it in the Q2, then **leave and reopen Library → Games → Q2 Arcade** so Q2 Pod scans the card again.

**Windows tip:** In File Explorer choose **View → Show → Hidden items** if `.arcade` is not visible.

**If Q2 Arcade still does not appear:** Confirm you are running Q2 Pod firmware with SD-card games support (stock Shanling firmware does not provide this custom Games discovery). The firmware scans hidden folders directly beneath the card root, requires a `q2app` file containing `row=games`, and only lists an app if its `q2-arcade` binary is present. If you installed other card games, verify the Arcade folder sits alongside them, not inside their folders.

The package contains `.arcade/q2app` (`name=Arcade`, `row=games`) and `.arcade/q2-arcade`, a freestanding MIPS32 little-endian program.

If you installed an older build, rename the SD-card folder `.q2-arcade` to `.arcade`; keep `q2-arcade` and `q2app` inside it. This also preserves `highscores`. The folder ID is `arcade`, so Q2 Pod looks for `.arcade/q2-arcade` and sends input to `/tmp/q2-arcade.input.sock`.

### Controls

The native Q2 Pod launcher contains nine games. Turn the wheel to scroll and press Centre to open a game. The firmware draws the list, status bar, theme and transitions. Touch selection is also supported.

| Game | Controls | Goal |
| --- | --- | --- |
| Sudoku | Wheel selects cells/digits, Centre edits/confirms | Solve the puzzle |
| Bricks | Wheel turns paddle, Centre launches | Clear circular brick rings |
| Solitaire | Wheel selects piles, Centre draws/picks/drops | Build foundations |
| Snake | Left/Right turns | Eat food and avoid collisions |
| Pong | Wheel or Left/Right moves paddle | Outscore the CPU |
| Tetris | Wheel rotates, Left/Right moves, Play soft-drops, Centre hard-drops | Clear rows |
| Breakout | Wheel or Left/Right moves paddle, Centre launches | Break all bricks |
| Battleship | Wheel selects a cell, Centre fires, or tap a cell | Sink four hidden ships (eight segments) in 26 shots |
| Minesweeper | Wheel, Left/Right, or tap to move the cursor; Centre reveals a cell, Play flags it | Clear the 9×9 board (10 mines) without hitting a mine |

Press Return/Back while playing to open the common pause menu. The games
use compact, original C graphics and game rules; no proprietary handheld
LCD game ROMs or artwork are included.

Within the pause menu, use the wheel or physical Left/Right buttons to select **Resume**, **Restart**, **Games**, or **Exit**, then press Centre. Return from the pause menu goes back to the arcade's game list. Touch taps on the game picker are supported.

**Physical button input:** The card manifest sets `audio=1` so Q2 Pod forwards the Left, Right, and Play/Pause hardware buttons to Arcade rather than the music player. This also stops music playback while Arcade is open. Despite the flag name, Arcade does not generate audio. The wheel rotates Blocks one step per roughly quarter turn, with large flicks limited to one rotation per input packet.

High scores for Tetris and Breakout are saved to `.arcade/highscores` on the SD card.

## Matching the Q2 Pod display theme

The **Arcade game-selection screen** reads the active theme from Q2 Pod's
`/mnt/data/config.ini` at launch. The same numeric Theme IDs and per-theme
Accent settings from [DiamondBond's theme sources](https://github.com/DiamondBond/q2-pod/tree/main/assets/themes)
are used:

- **Minimal:** themed background and accent-colored selection rows.
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
- `core/mini_games.c/.h` — Battleship and Minesweeper rules.
- `core/game_framework.c/.h` — shared lifecycle, input, update callback infrastructure.
- `src/app.c/.h` — arcade launcher and input/update adapters.
- `src/render.c` — lightweight frame drawing without AWTK dependencies.
- `src/platform_q2.c` — MIPS Linux framebuffer, input datagram socket, timekeeping and SD-card persistence.
- `tests/` — portable rules, framework and application-input regression tests.

The four original Q2 Pod games use the shared callback runner; other games use the card-level adapter.

For migration notes, implementation boundaries and contributor guidance, see [docs/architecture.md](docs/architecture.md). For the open-source algorithm references and new rendering details, see [docs/mini-games.md](docs/mini-games.md).

## Credits and license

Portable games and framework were developed for the `new-builtin-games` branch of Q2 Pod. Q2 card startup, input and frame-buffer integration follow the approach used in [DiamondBond's Q2 Fidget/Buddy](https://github.com/DiamondBond/q2-fidget), [Q2 2048](https://github.com/lampsonlnguyen/q2-2048) and [Q2 Dino Runner](https://github.com/lampsonlnguyen/q2-dino-runner). Original source remains GPL-3.0-or-later; see [LICENSE](LICENSE). No stock firmware or proprietary game binaries are distributed.

### Appearance

Arcade reads `/mnt/data/config.ini` when opened and follows Q2 Pod’s Minimal, Modern, or Classic theme, its saved accent, and Light Mode. Auto Light Mode inherits the visible firmware screen’s light/dark appearance at launch; reopen Arcade after a theme change or a day/night transition. Classic uses a glossy selection gradient. Game pieces keep distinct colors for readability. Bottom hints use compact 3×5 lettering centered inside the rounded screen.

Menu rows are compact (35-pixel spacing) to leave room for additional entries. Interface text uses smooth glyphs close to the original size, with centered headings and safe screen margins.

## Automatic builds and releases

GitHub Actions tests and builds each push to `main` and every pull request. Download the **q2-arcade-card-app** artifact from a successful run, or start a build with **Actions → q2-arcade → Run workflow**.

Push an unused version tag to publish a release automatically:

```sh
git pull --ff-only
git tag v1.0.1
git push origin v1.0.1
```

Once the build and package checks pass, Actions creates the release and attaches **q2-arcade.zip** directly. Re-running the tag build replaces that release's ZIP. Branch pushes and manual branch builds only save Actions artifacts.

### Wheel sensitivity

Arcade reads **System Settings → Controls → Wheel Sensitivity** (`[Q2POD] WHEELSENSITIVITY`) when opened. Menu and grid navigation use the firmware's tick threshold; paddle movement and Blocks rotation scale with the same percentage. Reopen Arcade after changing the setting. Missing or invalid values use Q2 Pod's default 50%.


## Q2 Pod 1.9.6 artwork

The installation ZIP includes a matching purple arcade cabinet icon at 128, 52 and 40 pixels, and a 96-pixel white-on-transparent Home Shortcut pane. Q2 Pod automatically uses `icon.png` (52 px), `icon-40.png` (40 px) and, in 1.9.6 or later, `pane.png` for a card app selected as Home's Shortcut. The 128-pixel `icon-128.png` is supplemental artwork, not a firmware row-icon filename. The pane is tinted by the active firmware theme. The images are generated during `make` by `tools/icons.py` using Python 3 without external libraries; run `python3 tools/icons.py --output dist/.arcade` to regenerate them. CI checks the actual packaged images. Older firmware can ignore the pane without affecting the game executable.

## Native game selection

Arcade follows Mixtape and Utilities using Q2SPOT/1 on /tmp/q2-arcade.browse.sock. Its manifest uses row=games rather than game=1 so firmware opens a native list. Each screen row launches the existing executable with Q2_URI; selecting Games, Exit or Back from pause returns to the native page. The menu process runs from RAM and never opens the framebuffer. Direct execution without Q2_URI retains the legacy launcher for diagnostics. The audio=1 flag gives each game the physical side buttons. Requires firmware supporting screen rows and row=games, including Q2 Pod 1.9.6.

## 8-Ball Pool

Play against the computer with a fixed-step integer physics simulation. Wheel aims; Left/Right fine-tune; hold Centre to charge and release to shoot; Play cycles power; tap the table to aim or place the cue after a foul. Return pauses. The game tracks solids/stripes, first contact, scratches, rail contact and legal eight-ball wins. This initial version uses simplified rules (no special break rerack or called pockets), a basic geometric CPU and no audio. Native launcher lists Pool last.

### Publish from GitHub Actions

Open Actions → Build and release q2-arcade → Run workflow, choose main and enter a new version such as `v1.6.0`. The workflow tests, builds and verifies the package, then creates a release and tag at the exact tested commit. Leave version empty to build without publishing. Existing tags/releases are rejected for manual runs. The existing tag-triggered workflow remains supported.
