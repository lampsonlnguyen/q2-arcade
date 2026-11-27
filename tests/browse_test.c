/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "app.h"
#include "browse.h"
#include <assert.h>
#include <string.h>
int main(void){
    char out[1024];
    size_t n=arcade_menu_reply("Q2SPOT/1\n42\tmenu\t\t\n",out,sizeof out);
    assert(n==strlen(out)&&n>0);
    const char *expected[]={"sudoku","bricks","solitaire","snake","pong","tetris","breakout","battleship","minesweeper"};
    const char *pos=out;
    for(int i=0;i<9;i++){
        const char *row=strstr(pos,expected[i]);assert(row&&row>pos);
        pos=row+strlen(expected[i]);
        int game=arcade_uri(expected[i]);assert(game==ARCADE_SUDOKU+i);
        ArcadeApp app;app_init(&app,42,0,0);
        assert(app_launch(&app,game)&&app.view==VIEW_PLAY&&app.native_menu);
        assert(app_input(&app,"k u 170")&&app.view==VIEW_PAUSE);
        app.option=2;assert(app_input(&app,"k u 218")&&app.quit);
    }
    assert(!strstr(pos,"screen\t"));
    assert(!arcade_menu_reply("garbage",out,sizeof out));
    assert(!arcade_menu_reply("Q2SPOT/1\n42\tmenu\n",out,8));
    assert(arcade_menu_reply("Q2SPOT/1\n42\tunknown\n",out,sizeof out)&&strstr(out,"unsupported"));
    char *env[]={"OTHER=1","Q2_URI=tetris",0};
    assert(!strcmp(arcade_env(env,"Q2_URI"),"tetris"));
    assert(!arcade_env(env,"Q2_UR"));
    assert(arcade_uri("unknown")==ARCADE_MENU);
    return 0;
}
