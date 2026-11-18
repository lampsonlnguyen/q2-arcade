/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "theme.h"
enum { MAX_CONFIG = 16384 };
static int whitespace(char c) { return c == ' ' || c == '\t' || c == '\r'; }
static char upper(char c) { return c >= 'a' && c <= 'z' ? (char)(c - 32) : c; }
static int key_is(const char *key, unsigned n, const char *expected) {
    unsigned i = 0;
    for (; i < n && expected[i]; ++i)
        if (upper(key[i]) != upper(expected[i])) return 0;
    return i == n && !expected[i];
}
static int decimal(const char *s, unsigned n) {
    if (!n || n > 3) return -1;
    int v = 0;
    for (unsigned i = 0; i < n; ++i) {
        if (s[i] < '0' || s[i] > '9') return -1;
        v = v * 10 + s[i] - '0';
    }
    return v;
}
void arcade_theme_defaults(ArcadeTheme *t) {
    t->wheel_percent = 50;
    t->style = ARCADE_MINIMAL;
    t->light = 0;
    t->accent = 0; /* Minimal preset: Graphite */
}
void arcade_theme_parse(ArcadeTheme *t, const char *config, unsigned size) {
    arcade_theme_defaults(t);
    if (!config || size > MAX_CONFIG) return;
    int section = 0, style = -1, light = -1, minimal = -1, modern = -1, classic = -1;
    unsigned at = 0;
    while (at < size) {
        unsigned from = at;
        while (at < size && config[at] != '\n' && config[at] != '\0') ++at;
        unsigned end = at;
        if (at < size && config[at] == '\n') ++at;
        if (at < size && config[at] == '\0') break;
        while (from < end && whitespace(config[from])) ++from;
        while (end > from && whitespace(config[end - 1])) --end;
        if (from == end || config[from] == '#' || config[from] == ';') continue;
        if (config[from] == '[' && config[end-1] == ']') {
            section = key_is(config + from + 1, end - from - 2, "IPOD") ? 1
                    : key_is(config + from + 1, end - from - 2, "Q2POD") ? 2 : 0;
            continue;
        }
        if (!section) continue;
        unsigned eq = from;
        while (eq < end && config[eq] != '=') ++eq;
        if (eq == end) continue;
        unsigned key_end = eq;
        while (key_end > from && whitespace(config[key_end - 1])) --key_end;
        unsigned value_at = eq + 1;
        while (value_at < end && whitespace(config[value_at])) ++value_at;
        int value = decimal(config + value_at, end - value_at);
        if (value < 0) continue;
        unsigned key_length = key_end - from;
        if(section==2){
            if(key_is(config+from,key_length,"WHEELSENSITIVITY") && value>=10 && value<=200 && value%10==0 && config[value_at]!='0')
                t->wheel_percent=value;
            continue;
        }
        if (key_is(config + from, key_length, "THEME") && value <= 2) style = value;
        else if (key_is(config + from, key_length, "LIGHT") && value <= 2) light = value;
        else if (key_is(config + from, key_length, "MINIMAL_ACCENT") && value <= 8) minimal = value;
        else if (key_is(config + from, key_length, "ACCENT") && value <= 8) modern = value;
        else if (key_is(config + from, key_length, "CLASSIC_LIGHT_ACCENT") && value <= 9) classic = value;
    }
    t->style = style < 0 ? ARCADE_MINIMAL : style;
    t->light = light < 0 ? 0 : light;
    /* The selected theme keeps its own accent setting. Classic Blue is 9. */
    t->accent = t->style == ARCADE_CLASSIC ? (classic < 0 ? 9 : classic)
              : t->style == ARCADE_MODERN ? (modern < 0 ? 0 : modern)
              : (minimal < 0 ? 0 : minimal);
}
unsigned arcade_theme_accent(const ArcadeTheme *t) {
    static const unsigned colors[] = {
        0x424242, /* Graphite */ 0xe8123f, /* Crimson */
        0x13838d, /* Tidal */ 0x8c732c, /* Champagne */
        0x2a6ad8, /* Cobalt */ 0xc45400, /* Tangerine */
        0x903868, /* Plum */ 0x424242, /* Ivory */
        0x647f34, /* Green */ 0x007dde /* Classic Blue */
    };
    return colors[t->accent >= 0 && t->accent < 10 ? t->accent : 0];
}
