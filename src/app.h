/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef Q2_ARCADE_APP_H
#define Q2_ARCADE_APP_H
#include <stdint.h>
#include "arcade_core.h"
#include "breakout_core.h"
#include "game_framework.h"
enum { SCREEN_W = 375, SCREEN_H = 320 };
enum { ARCADE_MENU, ARCADE_PONG, ARCADE_BLOCKS, ARCADE_BREAKOUT };
enum { VIEW_SELECT, VIEW_PLAY, VIEW_PAUSE, VIEW_OVER };
typedef uint32_t u32;
typedef struct {
    Q2Pong pong;
    Q2Blocks blocks;
    Q2Breakout breakout;
    unsigned best_blocks, best_breakout, seed, accumulator;
    int game, view, selection, option, wheel_remainder, dirty, quit;
} ArcadeApp;
void app_init(ArcadeApp *app, unsigned seed, unsigned best_blocks, unsigned best_breakout);
int app_input(ArcadeApp *app, const char *message);
int app_tick(ArcadeApp *app, unsigned elapsed_ms);
int app_animated(const ArcadeApp *app);
void app_theme(const char *config, int screen_light);
void app_render(const ArcadeApp *app, u32 *frame, unsigned stride);
#endif
