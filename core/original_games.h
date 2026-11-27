/* SPDX-License-Identifier: GPL-3.0-or-later
 * Game rules adapted from DiamondBond/q2-pod patch/games.c (fb23157cafc4e80b066a2c026ff9c220c7b29b4b).
 */
#ifndef Q2_ORIGINAL_GAMES_H
#define Q2_ORIGINAL_GAMES_H
#include "game_framework.h"
#define CLUE 16
#define RINGS 3
#define SEGS 16
#define R_B 40
#define BRICK_T 12
#define R_PAD 118
#define PAD_HALF 64
enum { AIM, PLAY, OVER };
#define UP 64
#define SOL_MAX 24
enum { STOCK, WASTE, FOUND, TABLEAU = FOUND + 4, PILES = TABLEAU + 7 };
#define SN_N 20
#define SN_CELLS (SN_N * SN_N)
struct sudoku {
    unsigned magic;
    unsigned char cell[81], cur, pad[2];
    unsigned sum;
};
struct bricks {
    int state, lives, level, score, pa, x, y, vx, vy;
    unsigned short brick[RINGS], pad;
};
struct solitaire {
    unsigned magic;
    unsigned char card[PILES][SOL_MAX], len[PILES], cur, pad[2];
    unsigned sum;
};
struct snake {
    unsigned short body[SN_CELLS], tail, len, food;
    unsigned char dir, turned, state; /* dir 0 up, 1 right, 2 down, 3 left; state PLAY, OVER */
    signed char next;                 /* a turn waiting for the next step, 0 none */
    int score;
};
enum { ORIGINAL_SUDOKU, ORIGINAL_BRICKS, ORIGINAL_SOLITAIRE, ORIGINAL_SNAKE, ORIGINAL_COUNT };
typedef struct {
    union { struct sudoku sud; struct bricks bricks; struct solitaire sol; struct snake snake; } state;
    unsigned seed, elapsed;
    int kind, picking, pick, won, ended, wheel_percent, wheel_remainder;
} OriginalGame;
extern const Q2GameHooks original_hooks;
int original_animated(const OriginalGame *g);
int original_sin(int angle);
int original_cos(int angle);
int original_angle(int y,int x);
#endif
