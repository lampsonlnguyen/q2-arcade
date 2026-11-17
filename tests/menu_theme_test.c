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
int main(void) {
    ArcadeApp app;
    app_init(&app, 42, 0, 0);
    /* Selected row, away from glyphs and borders. */
    app.theme.style = ARCADE_MINIMAL;
    app.theme.accent = 4;
    app_render(&app,frame,SCREEN_H);
    uint32_t minimal = pixel(145, 115);
    uint32_t menu_bg = pixel(185, 272);
    app.theme.style = ARCADE_MODERN;
    app_render(&app,frame,SCREEN_H);
    uint32_t modern = pixel(145, 115);
    assert(modern == arcade_theme_accent(&app.theme));
    assert(minimal != modern);
    app.theme.style = ARCADE_CLASSIC;
    app.theme.accent = 9;
    app_render(&app,frame,SCREEN_H);
    uint32_t classic = pixel(145, 115);
    assert(classic != minimal && classic != modern);
    assert(pixel(185,272) != menu_bg);
    /* A light-mode Minimal selection still respects the active accent. */
    app.theme.style=ARCADE_MINIMAL;
    app.theme.light=1;
    app.theme.accent=1;
    app_render(&app,frame,SCREEN_H);
    assert(pixel(145,115) != minimal);
    puts("menu_theme: all tests passed");
    return 0;
}
