/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "mini_games.h"
#include <assert.h>
#include <stdio.h>
static int select_turn(Q2Mini *g){return mini_select(g);}
static void battle(void){
    Q2Mini g;mini_init(&g,MINI_BATTLE,42);
    assert(g.target==8&&g.lives==26);
    int count=0;for(int y=0;y<6;y++)for(int x=0;x<6;x++)count+=g.ships[y][x];
    assert(count==8);
    assert(select_turn(&g));assert(g.moves==1);
    assert(!select_turn(&g));assert(g.moves==1);
    /* Every ship has an ID, and an eventual sink can be identified. */
    for(int id=1;id<=4;id++){
        int found=0;
        for(int y=0;y<6;y++)for(int x=0;x<6;x++)if(g.ship_id[y][x]==id)found++;
        assert(found==g.ship_length[id-1]);
    }
}
static void sinking(void){
    Q2Mini g;mini_init(&g,MINI_BATTLE,42);
    /* All segments of one small ship must raise a SUNK event. */
    int last=-1;
    for(int y=0;y<6;y++)for(int x=0;x<6;x++)
        if(g.ship_id[y][x]==4)last=y*6+x;
    assert(last>=0);
    g.cursor=last;
    assert(select_turn(&g));
    assert(g.last_result==3&&g.ship_hits[3]==1);
    assert(!select_turn(&g)); /* duplicate shots cannot consume ammo */
}
static void minesweeper(void){
    Q2Mini g;mini_init(&g,MINI_MINES,7);
    assert(g.kind==MINI_MINES&&g.mine_total==10&&g.cursor==40&&!g.started);
    /* The timer does not run before the first reveal. */
    assert(mini_tick(&g)&&g.elapsed_s==0);
    /* The first reveal is always safe and starts the timer. */
    assert(mini_select(&g)&&g.started&&!g.finished&&g.revealed_count>0);
    assert(mini_tick(&g)&&g.elapsed_s==1);
    /* Flag a hidden cell, try to reveal it while flagged, then unflag it. */
    int hidden=-1;
    for(int i=0;i<81;i++)if(!g.revealed[i/9][i%9]){hidden=i;break;}
    assert(hidden>=0);
    g.cursor=hidden;
    assert(mini_flag(&g)&&g.flags_placed==1&&g.flagged[hidden/9][hidden%9]);
    assert(!mini_select(&g));
    assert(mini_flag(&g)&&g.flags_placed==0);
    /* Hitting a mine ends the game as a loss; no moves after game over. */
    int mine=-1;
    for(int i=0;i<81;i++)if(g.mines[i/9][i%9]){mine=i;break;}
    assert(mine>=0);
    g.cursor=mine;
    assert(mini_select(&g)&&g.finished&&g.outcome==3&&!g.won);
    assert(!mini_select(&g)&&!mini_flag(&g));
    /* Winning: reveal every safe cell. */
    mini_init(&g,MINI_MINES,7);
    (void)mini_select(&g); /* place mines with a safe start */
    for(int i=0;i<81;i++){
        if(g.mines[i/9][i%9]||g.revealed[i/9][i%9])continue;
        g.cursor=i;(void)mini_select(&g);
    }
    assert(g.finished&&g.won&&g.outcome==1);
    assert(g.revealed_count==71);
    /* Flood fill: a zero cell uncovers its whole region at once. */
    mini_init(&g,MINI_MINES,7);
    (void)mini_select(&g);
    int zero=-1;
    for(int i=0;i<81;i++)
        if(!g.mines[i/9][i%9]&&g.adj[i/9][i%9]==0&&!g.revealed[i/9][i%9]){zero=i;break;}
    if(zero>=0){
        int before=g.revealed_count;
        g.cursor=zero;assert(mini_select(&g));
        assert(g.revealed_count>before+1);
    }
    /* Flag cap: at most ten flags, one per mine. */
    mini_init(&g,MINI_MINES,7);
    (void)mini_select(&g);
    int flagged=0;
    for(int i=0;i<81&&flagged<10;i++){
        if(g.revealed[i/9][i%9])continue;
        g.cursor=i;if(mini_flag(&g))flagged++;
    }
    assert(g.flags_placed==10);
    for(int i=0;i<81;i++){
        if(g.revealed[i/9][i%9]||g.flagged[i/9][i%9])continue;
        g.cursor=i;assert(!mini_flag(&g));break;
    }
}
int main(void){
    battle();sinking();minesweeper();
    puts("Handheld mini-game tests passed");
    return 0;
}
