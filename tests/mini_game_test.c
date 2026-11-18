/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "mini_games.h"
#include <assert.h>
#include <stdio.h>
static void tic(void){
    Q2Mini g;mini_init(&g,MINI_TTT,12);assert(g.kind==MINI_TTT);
    assert(mini_move(&g,1)&&g.cursor==1);
    assert(mini_select(&g)&&g.board[0][1]==1);
    int cpu=0;for(int y=0;y<3;y++)for(int x=0;x<3;x++)cpu+=g.board[y][x]==2;
    assert(cpu==1);assert(!mini_select(&g));
}
static void connect(void){
    Q2Mini g;mini_init(&g,MINI_CONNECT,18);
    assert(mini_move(&g,3)&&g.cursor==3);
    assert(mini_select(&g)&&g.board[5][3]==1);
    unsigned count=0;for(int y=0;y<6;y++)for(int x=0;x<7;x++)count+=g.board[y][x]==2;
    assert(count==1);
    /* Discs are circular at render time; the game still stacks to bottom. */
    assert(g.board[5][3]==1);
}
static void battle(void){
    Q2Mini g;mini_init(&g,MINI_BATTLE,42);
    assert(g.target==8&&g.lives==26);
    int count=0;for(int y=0;y<6;y++)for(int x=0;x<6;x++)count+=g.ships[y][x];
    assert(count==8);
    assert(mini_select(&g));assert(g.moves==1);
    assert(!mini_select(&g));assert(g.moves==1);
    /* Every ship has an ID, and an eventual sink can be identified. */
    for(int id=1;id<=4;id++){
        int found=0;
        for(int y=0;y<6;y++)for(int x=0;x<6;x++)if(g.ship_id[y][x]==id)found++;
        assert(found==g.ship_length[id-1]);
    }
}
static void lcd(void){
    for(int kind=MINI_ROAD;kind<=MINI_CATCH;kind++){
        Q2Mini g;mini_init(&g,kind,53);
        assert(mini_move(&g,1)&&g.lane==2);
        for(int i=0;i<24;i++)mini_tick(&g);
        assert(g.score+3-g.lives==2);
    }
}
int main(void){
    tic();connect();battle();lcd();
    puts("Handheld mini-game tests passed");
    return 0;
}
