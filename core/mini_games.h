/* SPDX-License-Identifier: GPL-3.0-or-later
 * Portable rules for five native handheld-inspired Q2 Arcade games.
 * The algorithms are adapted from open-source references documented
 * in docs/mini-games.md; no allocation or external runtime. */
#ifndef Q2_MINI_GAMES_H
#define Q2_MINI_GAMES_H
#include <stdint.h>
enum { MINI_TTT=0, MINI_CONNECT=1, MINI_BATTLE=2, MINI_ROAD=3, MINI_MINES=4 };
enum { MINI_MS_COLS=9, MINI_MS_ROWS=9, MINI_MS_MINES=10 };
typedef struct {
    uint8_t board[9][9]; /* tic: 1=X,2=O; connect: 1=red,2=yellow; battle: 1=hit,3=miss */
    uint8_t ships[6][6]; /* occupied squares, kept hidden until hit or game end */
    uint8_t ship_id[6][6]; /* 1..4 identifies each individual ship */
    uint8_t ship_length[4], ship_hits[4];
    uint8_t mines[MINI_MS_ROWS][MINI_MS_COLS]; /* minesweeper: 1=mine */
    uint8_t adj[MINI_MS_ROWS][MINI_MS_COLS]; /* minesweeper: adjacent mine count */
    uint8_t revealed[MINI_MS_ROWS][MINI_MS_COLS]; /* minesweeper: 1=revealed */
    uint8_t flagged[MINI_MS_ROWS][MINI_MS_COLS]; /* minesweeper: 1=flagged */
    int ttt_size, ttt_target, ttt_pvp, ttt_turn;
    uint32_t rng;
    unsigned score, moves, lives, target;
    int kind, cursor, lane, obstacle, obstacle_y, finished, won;
    int outcome; /* 0=running, 1=player win, 2=CPU win, 3=draw/loss */
    int last_result; /* battle: 1 hit, 2 miss, 3 sunk */
    int mine_total, flags_placed, revealed_count, started;
    unsigned elapsed_s; /* minesweeper: seconds since first reveal */
} Q2Mini;
void mini_init(Q2Mini *g,int kind,uint32_t seed);
void mini_ttt_init(Q2Mini *g,int size,int pvp,uint32_t seed);
int mini_move(Q2Mini *g,int steps);
int mini_select(Q2Mini *g);
int mini_flag(Q2Mini *g); /* minesweeper: toggle a flag on the cursor cell */
int mini_tick(Q2Mini *g);
#endif
