/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef Q2_ARCADE_APP_H
#define Q2_ARCADE_APP_H
#include <stdint.h>
#include "arcade_core.h"
#include "breakout_core.h"
#include "mini_games.h"
#include "game_framework.h"
#include "theme.h"
#include "original_games.h"
#include "pool_core.h"
enum { SCREEN_W = 375, SCREEN_H = 320 };
enum { MENU_TOP = 42, MENU_PITCH = 35, MENU_ROW_H = 35, MENU_VISIBLE = 7 };
enum { MS_CELL = 25, MS_BOARD_X = 75, MS_BOARD_Y = 64 };
enum { ARCADE_MENU, ARCADE_SUDOKU, ARCADE_BRICKS, ARCADE_SOLITAIRE, ARCADE_SNAKE, ARCADE_PONG, ARCADE_BLOCKS, ARCADE_BREAKOUT,
       ARCADE_BATTLE, ARCADE_MINES, ARCADE_POOL, ARCADE_GAME_COUNT };
enum { VIEW_SELECT, VIEW_PLAY, VIEW_PAUSE, VIEW_OVER };
typedef uint32_t u32;
typedef struct {
    OriginalGame original;
    Q2GameRunner runner;
    Q2Pong pong;
    Q2Blocks blocks;
    Q2Breakout breakout;
    Q2Mini mini;
    Q2Pool pool;
    ArcadeTheme theme;
    int wheel_pixel_remainder;
    unsigned best_blocks, best_breakout, seed, accumulator;
    int game, view, selection, option, wheel_remainder, dirty, quit, native_menu;
} ArcadeApp;
int app_launch(ArcadeApp *app,int game);
int app_menu_first(int selection);
void app_init(ArcadeApp *app, unsigned seed, unsigned best_blocks, unsigned best_breakout);
int app_input(ArcadeApp *app, const char *message);
int app_tick(ArcadeApp *app, unsigned elapsed_ms);
int app_animated(const ArcadeApp *app);
void app_theme(const char *config, int screen_light);
void app_render(const ArcadeApp *app, u32 *frame, unsigned stride);
#endif
