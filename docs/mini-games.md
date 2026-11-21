# Q2 Arcade handheld games: rules and graphics

This release replaces the first-pass generic grid/rectangle implementations of
Tic-Tac-Toe, Connect Four, Battleship, Road Dodge and Minesweeper.

## Open-source code studied

These projects are reference implementations for game rules, not linked
runtime dependencies. Q2 Arcade's implementation is a new compact C adaptation
built for the embedded Q2 card runtime; we did not import assets, frameworks,
network functions, UI libraries or large source trees.

- **Tic-Tac-Toe:** [namanyt/tictactoe.c](https://github.com/namanyt/tictactoe.c),
  MIT license, by Nitya Naman (2025). Studied terminal-state checking,
  recursively searching both players' moves, and depth-adjusted minimax scoring.
  Q2's `ttt_eval` and `ttt_cpu` use these rules with fixed-size board arrays,
  no heap, and deterministic best-move selection.
- **Connect Four:** [bymayanksingh/connect4](https://github.com/bymayanksingh/connect4),
  MIT license. Studied seven-column gravity, four-in-a-row evaluation, move
  ordering, minimax and alpha-beta pruning. Q2's `connect_search` uses bounded
  depth-4 search (counting the candidate CPU move) with open-line scoring and
  center-column priority, preserving a responsive small MIPS runtime.
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

- Tic-Tac-Toe: selectable 3x3, 6x6 and 9x9 playfields with packed transparent cyan X and gold O sprites; 3x3 uses perfect-play minimax, while 6x6 and 9x9 use bounded open-line scoring with immediate win/block detection. Local two-player mode alternates X and O.
- Connect Four: blue frame and dark circular sockets holding glossy red
  player discs and yellow CPU discs; the selected column is highlighted.
- Battleship: teal patterned ocean, a ship silhouette, visible shot splashes,
  red/orange hit markers, and steel ship segments revealed when sunk or at game
  end. The game displays hits, remaining torpedoes, and sink notifications.
- Road Dodge: grass verges, asphalt, scrolling lane dividers, and recognizable
  top-down cars with tires, windshields, cabins and lights.
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
