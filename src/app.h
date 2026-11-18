/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef Q2_ARCADE_APP_H
#define Q2_ARCADE_APP_H
#include <stdint.h>
#include "arcade_core.h"
#include "breakout_core.h"
#include "mini_games.h"
#include "game_framework.h"
#include "theme.h"
enum { SCREEN_W = 375, SCREEN_H = 320 };
enum { MENU_TOP = 64, MENU_PITCH = 38, MENU_ROW_H = 34 };
enum { ARCADE_MENU, ARCADE_PONG, ARCADE_BLOCKS, ARCADE_BREAKOUT,
       ARCADE_TTT, ARCADE_CONNECT, ARCADE_BATTLE, ARCADE_ROAD, ARCADE_CATCH, ARCADE_GAME_COUNT };
enum { VIEW_SELECT, VIEW_PLAY, VIEW_PAUSE, VIEW_OVER };
typedef uint32_t u32;
typedef struct {
    Q2Pong pong;
    Q2Blocks blocks;
    Q2Breakout breakout;
    Q2Mini mini;
    ArcadeTheme theme;
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
