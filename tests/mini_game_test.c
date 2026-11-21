/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "mini_games.h"
#include <assert.h>
#include <stdio.h>
static int select_turn(Q2Mini *g){
    int acted=mini_select(g);
    if(acted&&g->kind==MINI_TTT&&!g->ttt_pvp&&g->ttt_turn==2)(void)mini_tick(g);
    return acted;
}
static void tic(void){
    Q2Mini g;mini_init(&g,MINI_TTT,12);assert(g.kind==MINI_TTT);
    assert(mini_move(&g,1)&&g.cursor==1);
    assert(select_turn(&g)&&g.board[0][1]==1);
    int cpu=0;for(int y=0;y<3;y++)for(int x=0;x<3;x++)cpu+=g.board[y][x]==2;
    assert(cpu==1);assert(!select_turn(&g));
}
static void connect(void){
    Q2Mini g;mini_init(&g,MINI_CONNECT,18);
    assert(mini_move(&g,3)&&g.cursor==3);
    assert(select_turn(&g)&&g.board[5][3]==1);
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
    assert(select_turn(&g));assert(g.moves==1);
    assert(!select_turn(&g));assert(g.moves==1);
    /* Every ship has an ID, and an eventual sink can be identified. */
    for(int id=1;id<=4;id++){
        int found=0;
        for(int y=0;y<6;y++)for(int x=0;x<6;x++)if(g.ship_id[y][x]==id)found++;
        assert(found==g.ship_length[id-1]);
    }
}
static void tactical_ai(void){
    /* Tic-Tac-Toe CPU chooses a forced win, not a random corner. */
    Q2Mini g;mini_init(&g,MINI_TTT,4);
    g.board[0][0]=2;g.board[0][1]=2;
    g.board[1][0]=1;g.board[1][1]=1;
    g.moves=4;g.cursor=8;
    assert(select_turn(&g));
    assert(g.board[0][2]==2&&g.outcome==2&&g.finished);
    /* Connect Four CPU completes an immediate horizontal four. */
    mini_init(&g,MINI_CONNECT,4);
    for(int c=0;c<3;c++)g.board[5][c]=2;
    g.moves=3;g.cursor=6;
    assert(select_turn(&g));
    assert(g.board[5][3]==2&&g.outcome==2&&g.finished);
    /* When player threatens four, CPU must block the next square. */
    mini_init(&g,MINI_CONNECT,4);
    for(int c=0;c<3;c++)g.board[5][c]=1;
    g.moves=3;g.cursor=6;
    assert(select_turn(&g));
    assert(g.board[5][3]==2&&!g.finished);
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
static void lcd(void){
    Q2Mini g;mini_init(&g,MINI_ROAD,53);
    assert(mini_move(&g,1)&&g.lane==2);
    for(int i=0;i<24;i++)mini_tick(&g);
    assert(g.score+3-g.lives==2);
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
static void tic_sizes(void){
    for(int size=3;size<=9;size+=3){
        Q2Mini g;mini_ttt_init(&g,size,1,1);
        int k=g.ttt_target;
        /* Alternating local turns, rejected occupied move preserves the turn. */
        assert(select_turn(&g)&&g.moves==1&&g.ttt_turn==2);
        assert(!select_turn(&g)&&g.ttt_turn==2);
        g.cursor=size;assert(select_turn(&g)&&g.board[1][0]==2&&g.moves==2);
        /* A winning run may begin away from the top/left on large boards. */
        mini_ttt_init(&g,size,1,1);
        for(int x=size-k;x<size-1;x++)g.board[size-1][x]=1;
        g.moves=(unsigned)(k-1);g.cursor=size*size-1;
        assert(select_turn(&g)&&g.finished&&g.outcome==1);
        mini_ttt_init(&g,size,1,1);g.ttt_turn=2;
        for(int i=1;i<k;i++)g.board[size-i][i-1]=2;
        g.cursor=(size-k)*size+k-1;
        assert(select_turn(&g)&&g.finished&&g.outcome==2);
        /* Full boards draw rather than hand another turn to the AI. */
        mini_ttt_init(&g,size,1,1);
        for(int y=0;y<size;y++)for(int x=0;x<size;x++)
            g.board[y][x]=(uint8_t)(1+((x+2*y)%4)/2);
        g.board[0][0]=0;g.moves=(unsigned)(size*size-1);
        assert(select_turn(&g)&&g.finished&&g.outcome==3);
        mini_ttt_init(&g,size,1,1);assert(mini_move(&g,-1)&&g.cursor==size*size-1);
        if(size==3)continue;
        /* Larger AI completes its win first and blocks an immediate X win. */
        mini_ttt_init(&g,size,0,1);
        for(int x=0;x<k-1;x++)g.board[size-1][x]=2;
        g.cursor=0;assert(select_turn(&g)&&g.outcome==2&&g.finished);
        mini_ttt_init(&g,size,0,1);
        for(int x=0;x<k-1;x++)g.board[size-1][x]=1;
        g.cursor=0;assert(select_turn(&g)&&g.board[size-1][k-1]==2);
    }
}
int main(void){
    tic_sizes();tic();connect();battle();tactical_ai();sinking();lcd();minesweeper();
    puts("Handheld mini-game tests passed");
    return 0;
}
