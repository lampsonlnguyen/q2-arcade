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
static void tactical_ai(void){
    /* Tic-Tac-Toe CPU chooses a forced win, not a random corner. */
    Q2Mini g;mini_init(&g,MINI_TTT,4);
    g.board[0][0]=2;g.board[0][1]=2;
    g.board[1][0]=1;g.board[1][1]=1;
    g.moves=4;g.cursor=8;
    assert(mini_select(&g));
    assert(g.board[0][2]==2&&g.outcome==2&&g.finished);
    /* Connect Four CPU completes an immediate horizontal four. */
    mini_init(&g,MINI_CONNECT,4);
    for(int c=0;c<3;c++)g.board[5][c]=2;
    g.moves=3;g.cursor=6;
    assert(mini_select(&g));
    assert(g.board[5][3]==2&&g.outcome==2&&g.finished);
    /* When player threatens four, CPU must block the next square. */
    mini_init(&g,MINI_CONNECT,4);
    for(int c=0;c<3;c++)g.board[5][c]=1;
    g.moves=3;g.cursor=6;
    assert(mini_select(&g));
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
    assert(mini_select(&g));
    assert(g.last_result==3&&g.ship_hits[3]==1);
    assert(!mini_select(&g)); /* duplicate shots cannot consume ammo */
}
static void lcd(void){
    for(int kind=MINI_ROAD;kind<=MINI_CATCH;kind++){
        Q2Mini g;mini_init(&g,kind,53);
        assert(mini_move(&g,1)&&g.lane==2);
        for(int i=0;i<24;i++)mini_tick(&g);
        assert(g.score+3-g.lives==2);
    }
}
static void tic_sizes(void){
    for(int size=3;size<=9;size+=3){
        Q2Mini g;mini_ttt_init(&g,size,1,1);
        int k=g.ttt_target;
        /* Alternating local turns, rejected occupied move preserves the turn. */
        assert(mini_select(&g)&&g.moves==1&&g.ttt_turn==2);
        assert(!mini_select(&g)&&g.ttt_turn==2);
        g.cursor=size;assert(mini_select(&g)&&g.board[1][0]==2&&g.moves==2);
        /* A winning run may begin away from the top/left on large boards. */
        mini_ttt_init(&g,size,1,1);
        for(int x=size-k;x<size-1;x++)g.board[size-1][x]=1;
        g.moves=(unsigned)(k-1);g.cursor=size*size-1;
        assert(mini_select(&g)&&g.finished&&g.outcome==1);
        mini_ttt_init(&g,size,1,1);g.ttt_turn=2;
        for(int i=1;i<k;i++)g.board[size-i][i-1]=2;
        g.cursor=(size-k)*size+k-1;
        assert(mini_select(&g)&&g.finished&&g.outcome==2);
        /* Full boards draw rather than hand another turn to the AI. */
        mini_ttt_init(&g,size,1,1);
        for(int y=0;y<size;y++)for(int x=0;x<size;x++)
            g.board[y][x]=(uint8_t)(1+((x+2*y)%4)/2);
        g.board[0][0]=0;g.moves=(unsigned)(size*size-1);
        assert(mini_select(&g)&&g.finished&&g.outcome==3);
        mini_ttt_init(&g,size,1,1);assert(mini_move(&g,-1)&&g.cursor==size*size-1);
        if(size==3)continue;
        /* Larger AI completes its win first and blocks an immediate X win. */
        mini_ttt_init(&g,size,0,1);
        for(int x=0;x<k-1;x++)g.board[size-1][x]=2;
        g.cursor=0;assert(mini_select(&g)&&g.outcome==2&&g.finished);
        mini_ttt_init(&g,size,0,1);
        for(int x=0;x<k-1;x++)g.board[size-1][x]=1;
        g.cursor=0;assert(mini_select(&g)&&g.board[size-1][k-1]==2);
    }
}
int main(void){
    tic_sizes();tic();connect();battle();tactical_ai();sinking();lcd();
    puts("Handheld mini-game tests passed");
    return 0;
}
