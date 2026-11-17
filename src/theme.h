/* SPDX-License-Identifier: GPL-3.0-or-later
 * Theme-aware menu styling, independent of firmware UI internals.
 * Mirrors Q2 Pod's IPOD/THEME IDs and accent order.
 */
#ifndef Q2_ARCADE_THEME_H
#define Q2_ARCADE_THEME_H
enum { ARCADE_MINIMAL = 0, ARCADE_CLASSIC = 1, ARCADE_MODERN = 2 };
typedef struct {
    int style;             /* Q2 Pod THEME: 0 Minimal, 1 Classic, 2 Modern */
    int light;             /* LIGHT: 0 dark, 1 light; 2 auto uses dark fallback */
    int accent;            /* active theme's accent (0-9) */
} ArcadeTheme;
void arcade_theme_defaults(ArcadeTheme *theme);
/* Parse one read-only snapshot of /mnt/data/config.ini. Never writes it.
 * On a malformed or missing value, retain Q2 Pod-compatible defaults.
 * Safe to call in native tests and on the freestanding MIPS card.
 */
void arcade_theme_parse(ArcadeTheme *theme, const char *config, unsigned size);
unsigned arcade_theme_accent(const ArcadeTheme *theme);
#endif
