/* SPDX-License-Identifier: GPL-3.0-or-later
 * Portable rules for five native handheld-inspired Q2 Arcade games.
 * The algorithms are adapted from open-source references documented
 * in docs/mini-games.md; no allocation or external runtime. */
#ifndef Q2_MINI_GAMES_H
#define Q2_MINI_GAMES_H
#include <stdint.h>
enum { MINI_TTT=0, MINI_CONNECT=1, MINI_BATTLE=2, MINI_ROAD=3, MINI_CATCH=4 };
typedef struct {
    uint8_t board[6][7]; /* tic: 1=X,2=O; connect: 1=red,2=yellow; battle: 1=hit,3=miss */
    uint8_t ships[6][6]; /* occupied squares, kept hidden until hit or game end */
    uint8_t ship_id[6][6]; /* 1..4 identifies each individual ship */
    uint8_t ship_length[4], ship_hits[4];
    uint32_t rng;
    unsigned score, moves, lives, target;
    int kind, cursor, lane, obstacle, obstacle_y, finished, won;
    int outcome; /* 0=running, 1=player win, 2=CPU win, 3=draw/loss */
    int last_result; /* battle: 1 hit, 2 miss, 3 sunk */
} Q2Mini;
void mini_init(Q2Mini *g,int kind,uint32_t seed);
int mini_move(Q2Mini *g,int steps);
int mini_select(Q2Mini *g);
int mini_tick(Q2Mini *g);
#endif
