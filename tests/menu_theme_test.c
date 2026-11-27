/* SPDX-License-Identifier: GPL-3.0-or-later
 * Smoke test all three theme-specific Arcade launcher layouts.
 */
#include "../src/app.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

static uint32_t frame[SCREEN_W*SCREEN_H];
static uint32_t pixel(unsigned x, unsigned y) {
    assert(x < SCREEN_W && y < SCREEN_H);
    return frame[x*SCREEN_H + SCREEN_H - 1 - y] & 0xffffffu;
}
/* Regression tests: the new games use identifiable original artwork
 * rather than generic monochrome rectangles. */
static int count_color(uint32_t color) {
    int n=0;
    for(int i=0;i<SCREEN_W*SCREEN_H;i++)
        n+=(frame[i]&0xffffffu)==color;
    return n;
}
static void game_art(void) {
    ArcadeApp app;
    app_init(&app,88,0,0);
    app.view=VIEW_PLAY;
    app.game=ARCADE_BATTLE;
    mini_init(&app.mini,MINI_BATTLE,12);
    app_render(&app,frame,SCREEN_H);
    assert(count_color(0x075070)>10); /* sea */
    assert(count_color(0x9ebbc4)>10); /* ship */
    app.game=ARCADE_MINES;
    mini_init(&app.mini,MINI_MINES,12);
    app_render(&app,frame,SCREEN_H);
    assert(count_color(0x191919)>10000); /* unrevealed cells */
    app.mini.flagged[0][0]=1;
    app_render(&app,frame,SCREEN_H);
    assert(count_color(0xe83d42)>25); /* flag */
    (void)mini_select(&app.mini);
    app_render(&app,frame,SCREEN_H);
    assert(count_color(0x11161c)>100); /* uncovered cells */
}
int main(void) {
    ArcadeApp app;
    app_init(&app, 42, 0, 0);
    /* Selected row, away from glyphs and borders. */
    app.theme.style = ARCADE_MINIMAL;
    app.theme.accent = 0;
    app_render(&app,frame,SCREEN_H);
    uint32_t minimal = pixel(145, 52);
    uint32_t menu_bg = pixel(185, 298);
    app.theme.style = ARCADE_MODERN;
    app.theme.accent = 4;
    app_render(&app,frame,SCREEN_H);
    uint32_t modern = pixel(145, 52);
    assert(modern == arcade_theme_accent(&app.theme));
    assert(minimal != modern);
    app.theme.style = ARCADE_CLASSIC;
    app.theme.accent = 9;
    app_render(&app,frame,SCREEN_H);
    uint32_t classic = pixel(145, 52);
    assert(classic != minimal && classic != modern);
    assert(pixel(185,298) != menu_bg);
    /* A light-mode Minimal selection still respects the active accent. */
    app.theme.style=ARCADE_MINIMAL;
    app.theme.light=1;
    app.theme.accent=1;
    app_render(&app,frame,SCREEN_H);
    assert(pixel(145,52) != minimal);
    game_art();
    puts("menu_theme: all tests passed");
    return 0;
}
