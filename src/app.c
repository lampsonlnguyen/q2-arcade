/* SPDX-License-Identifier: GPL-3.0-or-later
 * Single card app, shared wheel/button routing and per-game portable rules.
 * No firmware patch or bespoke MIPS game adapter required.
 */
#include "app.h"
enum { KEY_RETURN = 170, KEY_PLAY = 171, KEY_PREV = 172, KEY_NEXT = 173,
       KEY_CENTER = 218, KEY_LEFT = 222, KEY_RIGHT = 223 };
/* Q2 reports about 200 wheel units per full turn. One quarter turn rotates
 * Blocks one step, rather than ten rotations per revolution. */
enum { WHEEL_STEP = 50 };
static unsigned next_seed(ArcadeApp *a) { return a->seed = a->seed * 1664525u + 1013904223u; }
void app_init(ArcadeApp *a, unsigned seed, unsigned best_blocks, unsigned best_breakout) {
    *a = (ArcadeApp){0};
    a->seed = seed ? seed : 42u;
    a->best_blocks = best_blocks;
    a->best_breakout = best_breakout;
}
static void start(ArcadeApp *a, int game) {
    a->game = game;
    a->view = VIEW_PLAY;
    a->wheel_remainder = 0;
    a->accumulator = 0;
    if (game == ARCADE_PONG) q2_pong_init(&a->pong);
    else if (game == ARCADE_BLOCKS) q2_blocks_init(&a->blocks, next_seed(a));
    else if (game == ARCADE_BREAKOUT) q2_breakout_init(&a->breakout);
}
static void save_best(ArcadeApp *a) {
    if (a->game == ARCADE_BLOCKS && a->blocks.score > a->best_blocks) {
        a->best_blocks = a->blocks.score; a->dirty = 1;
    }
    if (a->game == ARCADE_BREAKOUT && a->breakout.score > a->best_breakout) {
        a->best_breakout = a->breakout.score; a->dirty = 1;
    }
}
static void finish(ArcadeApp *a) {
    if ((a->game == ARCADE_BLOCKS && a->blocks.game_over) ||
        (a->game == ARCADE_BREAKOUT && a->breakout.game_over)) {
        save_best(a);
        a->view = VIEW_OVER;
        a->option = 0;
    }
}
static int select_option(ArcadeApp *a) {
    if (a->view == VIEW_SELECT) {
        start(a, ARCADE_PONG + a->selection);
    } else if (a->view == VIEW_PAUSE) {
        if (a->option == 0) a->view = VIEW_PLAY;
        else if (a->option == 1) start(a, a->game);
        else if (a->option == 2) { save_best(a); a->view = VIEW_SELECT; }
        else { save_best(a); a->quit = 1; }
    } else if (a->view == VIEW_OVER) {
        if (a->option == 0) start(a, a->game);
        else { save_best(a); a->view = VIEW_SELECT; }
    } else if (a->game == ARCADE_BLOCKS) {
        (void)q2_blocks_drop(&a->blocks);
        finish(a);
    } else if (a->game == ARCADE_BREAKOUT) {
        q2_breakout_launch(&a->breakout);
    }
    return 1;
}
static int shift_option(ArcadeApp *a, int direction) {
    int count = a->view == VIEW_SELECT ? 3 : a->view == VIEW_PAUSE ? 4 : 2;
    int *choice = a->view == VIEW_SELECT ? &a->selection : &a->option;
    *choice = (*choice + (direction > 0 ? 1 : count - 1)) % count;
    return 1;
}
static int wheel(ArcadeApp *a, int movement) {
    if (movement > 1000) movement = 1000;
    if (movement < -1000) movement = -1000;
    if (!movement) return 0;
    /* Q2 input protocol gives negative travel for clockwise motion. */
    if (a->view == VIEW_PLAY && a->game == ARCADE_PONG) {
        q2_pong_move(&a->pong, movement * -2); return 1;
    }
    if (a->view == VIEW_PLAY && a->game == ARCADE_BREAKOUT) {
        q2_breakout_move(&a->breakout, movement * -2); return 1;
    }
    a->wheel_remainder -= movement;
    int steps = a->wheel_remainder / WHEEL_STEP;
    a->wheel_remainder %= WHEEL_STEP;
    if (a->view == VIEW_PLAY && a->game == ARCADE_BLOCKS) {
        /* A fast wheel flick must not spin a piece multiple times in a
         * single UI frame. One turn per packet is predictable on hardware. */
        if (steps) {
            (void)q2_blocks_rotate_dir(&a->blocks, steps > 0 ? 1 : -1);
            /* Do not carry the unused portion of a flick into the next,
             * possibly opposite, turn. */
            a->wheel_remainder = 0;
        }
        return steps != 0;
    }
    if (steps > 8) steps = 8;
    if (steps < -8) steps = -8;
    for (int i = 0; i < (steps < 0 ? -steps : steps); ++i)
        shift_option(a, steps > 0 ? 1 : -1);
    return steps != 0;
}
static int key(ArcadeApp *a, int button) {
    if (button == KEY_RETURN) {
        if (a->view == VIEW_SELECT) a->quit = 1;
        else if (a->view == VIEW_PLAY) { a->view = VIEW_PAUSE; a->option = 0; }
        else if (a->view == VIEW_PAUSE) { save_best(a); a->view = VIEW_SELECT; }
        else a->view = VIEW_SELECT;
        a->wheel_remainder = 0;
        return 1;
    }
    if (button == KEY_CENTER) return select_option(a);
    if (a->view != VIEW_PLAY) {
        if (button == KEY_LEFT || button == KEY_PREV) return shift_option(a, -1);
        if (button == KEY_RIGHT || button == KEY_NEXT) return shift_option(a, 1);
        return 0;
    }
    if (a->game == ARCADE_BLOCKS) {
        if (button == KEY_LEFT) return q2_blocks_move(&a->blocks, -1);
        if (button == KEY_RIGHT) return q2_blocks_move(&a->blocks, 1);
        if (button == KEY_PLAY) {
            /* The physical bottom button is Down; accelerate, don't pause. */
            for (int i = 0; i < 3 && !a->blocks.game_over; ++i)
                if (q2_blocks_tick(&a->blocks) != 0) break;
            finish(a);
            return 1;
        }
    } else if (a->game == ARCADE_PONG) {
        if (button == KEY_LEFT) { q2_pong_move(&a->pong, -14); return 1; }
        if (button == KEY_RIGHT) { q2_pong_move(&a->pong, 14); return 1; }
        if (button == KEY_PLAY) { a->view = VIEW_PAUSE; a->option = 0; return 1; }
    } else if (a->game == ARCADE_BREAKOUT) {
        if (button == KEY_LEFT) { q2_breakout_move(&a->breakout, -14); return 1; }
        if (button == KEY_RIGHT) { q2_breakout_move(&a->breakout, 14); return 1; }
        if (button == KEY_PLAY) { a->view = VIEW_PAUSE; a->option = 0; return 1; }
    }
    return 0;
}
static int number(const char **at) {
    const char *p = *at;
    while (*p == ' ') ++p;
    int sign = 1;
    if (*p == '-') { sign = -1; ++p; }
    if (*p < '0' || *p > '9') return 2000000;
    int n = 0;
    while (*p >= '0' && *p <= '9') {
        if (n > 100000) return 2000000;
        n = n * 10 + (*p++ - '0');
    }
    *at = p;
    return n * sign;
}
int app_input(ArcadeApp *a, const char *message) {
    if (!message || !message[0] || message[1] != ' ') return 0;
    if (message[0] == 'w') {
        const char *p = message + 2;
        int movement = number(&p);
        return movement >= -2000 && movement <= 2000 ? wheel(a, movement) : 0;
    }
    if (message[2] != ' ' && message[3] != ' ') return 0;
    if (!message[2] || message[3] != ' ') return 0;
    const char *p = message + 4;
    int x = number(&p);
    if (message[0] == 'k' && message[2] == 'u' && x < 2000000)
        return key(a, x);
    if (message[0] == 't' && message[2] == 'u') {
        int y = number(&p);
        if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return 0;
        if (a->view == VIEW_SELECT && y >= 86 && y < 260) {
            a->selection = (y - 86) / 58;
            if (a->selection > 2) a->selection = 2;
            return select_option(a);
        }
        if (a->view != VIEW_PLAY && y >= 105 && y < 284) {
            int count = a->view == VIEW_PAUSE ? 4 : 2;
            int index = (y - 105) / (count == 4 ? 43 : 75);
            if (index >= 0 && index < count) { a->option = index; return select_option(a); }
        }
        if (a->view == VIEW_PLAY) {
            if (y < 48) return key(a, KEY_RETURN);
            if (a->game == ARCADE_BLOCKS) {
                if (x < 125) return key(a, KEY_LEFT);
                if (x > 235) return key(a, KEY_RIGHT);
                return key(a, KEY_PLAY);
            }
            if (a->game == ARCADE_BREAKOUT) {
                q2_breakout_move(&a->breakout, x - 27 - a->breakout.paddle_x - 24);
                return 1;
            }
            q2_pong_move(&a->pong, y - 65 - a->pong.player_y - 24);
            return 1;
        }
    }
    return 0;
}
int app_animated(const ArcadeApp *a) { return a->view == VIEW_PLAY; }
int app_tick(ArcadeApp *a, unsigned elapsed_ms) {
    if (!app_animated(a)) return 0;
    if (elapsed_ms > 100) elapsed_ms = 100;
    if (a->game == ARCADE_PONG || a->game == ARCADE_BREAKOUT) {
        a->accumulator += elapsed_ms * 60u;
        for (int n = 0; n < 6 && a->accumulator >= 1000u; ++n) {
            a->accumulator -= 1000u;
            if (a->game == ARCADE_PONG) q2_pong_tick(&a->pong);
            else q2_breakout_tick(&a->breakout);
        }
    } else if (a->game == ARCADE_BLOCKS) {
        unsigned steps = a->blocks.level < 13 ? a->blocks.level - 1 : 12;
        unsigned delay = 650u - steps * 45u;
        if (delay < 85u) delay = 85u;
        a->accumulator += elapsed_ms;
        if (a->accumulator >= delay) {
            a->accumulator %= delay;
            (void)q2_blocks_tick(&a->blocks);
        }
    }
    finish(a);
    return 1;
}
