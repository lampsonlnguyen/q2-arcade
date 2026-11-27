# Q2 Arcade handheld games: rules and graphics

This release replaces the first-pass generic grid/rectangle implementations of
Battleship and Minesweeper.

## Open-source code studied

These projects are reference implementations for game rules, not linked
runtime dependencies. Q2 Arcade's implementation is a new compact C adaptation
built for the embedded Q2 card runtime; we did not import assets, frameworks,
network functions, UI libraries or large source trees.

- **Battleship:** [sehundpark/battleship](https://github.com/sehundpark/battleship),
  MIT license. Studied validating ship placement, individual ship IDs, tracking
  successful attacks and checking when each ship is sunk. Q2 uses a 6x6
  single-player search challenge with 4 ships of lengths 3, 2, 2, and 1.
  Every hidden ship has its own hit counter, so a player can be told when a
  particular ship sinks. The player gets 26 shots.
- **Minesweeper:** [Rockbox `apps/plugins/minesweeper.c`](https://github.com/Rockbox/rockbox/blob/master/apps/plugins/minesweeper.c),
  GPL-2.0-or-later, by Antoine Cellerier (2004-2006). Studied tile state
  (mine / flag / known / neighbour count), placing mines away from the first
  revealed cell so the opening move is always safe, flood-fill uncovering of
  empty regions, and win/lose detection. Q2 uses a 9x9 beginner board with
  10 mines, a mine counter, and a one-second timer that starts on the first
  reveal. [tsoding/mine](https://github.com/tsoding/mine), MIT license, was a
  secondary reference for the same rules in a minimal C implementation.

The original implementation is licensed as GPL-3.0-or-later, see
[LICENSE](../LICENSE). These references are credited for algorithms and
architecture; their existing graphics, sound effects, and source files have
not been bundled.

## Native Q2 graphics

All artwork is rendered directly to the framebuffer using bounded integer
pixel primitives in `src/render.c`:

- Battleship: teal patterned ocean, a ship silhouette, visible shot splashes,
  red/orange hit markers, and steel ship segments revealed when sunk or at game
  end. The game displays hits, remaining torpedoes, and sink notifications.
- Minesweeper: a 9x9 grid of raised cells on a dark board with a gold cursor,
  classic coloured neighbour counts, red flags, and spiked mines revealed on
  loss (the mine hit is highlighted red); the HUD shows mines remaining and
  elapsed time.

All touch targets follow these drawn bounds. Menu navigation and the existing
Pong, Blocks, Breakout rules are unchanged.

## Testing

`make check` covers rule invariants, the device input router, and distinct
colored-pixel assertions for each new illustration. `make` compiles the same
C implementation into a MIPS Q2 binary without any new runtime dependencies.

Artwork geometry and game pacing should also be checked on a physical Q2.
