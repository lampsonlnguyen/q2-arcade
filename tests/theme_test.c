/* SPDX-License-Identifier: GPL-3.0-or-later
 * Q2 Pod active theme settings: numeric IDs from DiamondBond theme XML.
 */
#include "../src/theme.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static ArcadeTheme parse(const char *config) {
    ArcadeTheme t;
    arcade_theme_parse(&t, config, (unsigned)strlen(config));
    return t;
}
int main(void) {
    ArcadeTheme t;
    arcade_theme_defaults(&t);
    assert(t.style==ARCADE_MINIMAL && t.accent==0 && t.light==0);
    t=parse("[IPOD]\nTHEME=2\nACCENT=4\nMINIMAL_ACCENT=1\nLIGHT=1\n");
    assert(t.style==ARCADE_MODERN && t.accent==4 && t.light==1);
    assert(arcade_theme_accent(&t)==0x2a6ad8u);
    t=parse("[OTHER]\nTHEME=2\n[IPOD]\nTHEME=1\nCLASSIC_LIGHT_ACCENT=9\nACCENT=2\n");
    assert(t.style==ARCADE_CLASSIC && t.accent==9);
    assert(arcade_theme_accent(&t)==0x007ddeu);
    t=parse(" [ipod] \r\n THEME = 0 \r\n MINIMAL_ACCENT = 7 \r\n");
    assert(t.style==ARCADE_MINIMAL && t.accent==7);
    t=parse("[IPOD]\nTHEME=42\nACCENT=8\n");
    assert(t.style==ARCADE_MINIMAL && t.accent==0);
    t=parse("[IPOD]\nTHEME=1\n");
    assert(t.style==ARCADE_CLASSIC && t.accent==9);
    t=parse("[IPOD]\nTHEME=2\n");
    assert(t.style==ARCADE_MODERN && t.accent==0);
    t=parse("[IPOD]\nTHEME=0\nLIGHT=2\n");
    assert(t.light==2); /* Auto mode: card uses dark until firmware state is available */
    t=parse("[OTHER]\nTHEME=1\n");
    assert(t.style==ARCADE_MINIMAL);
    arcade_theme_parse(&t,0,0);
    assert(t.style==ARCADE_MINIMAL && t.accent==0);
    t=parse("[Q2POD]\nWHEELSENSITIVITY=10\n[IPOD]\nTHEME=2\n");
    assert(t.wheel_percent==10&&t.style==ARCADE_MODERN);
    t=parse("[Q2POD]\nWHEELSENSITIVITY=200\n");assert(t.wheel_percent==200);
    t=parse("[IPOD]\nWHEELSENSITIVITY=100\n");assert(t.wheel_percent==50);
    const char *bad[]={"0","9","201","55","010","oops"};
    for(unsigned i=0;i<sizeof bad/sizeof bad[0];i++){
        char config[80];snprintf(config,sizeof config,"[Q2POD]\nWHEELSENSITIVITY=%s\n",bad[i]);
        t=parse(config);assert(t.wheel_percent==50);
    }
    puts("theme: all tests passed");
    return 0;
}
