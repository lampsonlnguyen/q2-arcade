/* SPDX-License-Identifier: GPL-3.0-or-later
 * Small self-contained handheld game rules, no platform or heap dependencies. */
#ifndef Q2_MINI_GAMES_H
#define Q2_MINI_GAMES_H
#include <stdint.h>
enum { MINI_TTT=0, MINI_CONNECT=1, MINI_BATTLE=2, MINI_ROAD=3, MINI_CATCH=4 };
typedef struct {
    uint8_t board[6][7]; /* X / player = 1, CPU = 2; shot miss = 3 */
    uint8_t ships[6][6]; /* Hidden from Battleship renderer. */
    uint32_t rng;
    unsigned score, moves, lives, target;
    int kind, cursor, lane, obstacle, obstacle_y, finished, won;
} Q2Mini;
void mini_init(Q2Mini *g,int kind,uint32_t seed);
int mini_move(Q2Mini *g,int steps);
int mini_select(Q2Mini *g);
int mini_tick(Q2Mini *g);
#endif
