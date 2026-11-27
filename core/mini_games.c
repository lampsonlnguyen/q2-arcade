/* SPDX-License-Identifier: GPL-3.0-or-later
 * Battleship: randomized non-overlapping ships, hits, misses, sunk status
 * (sehundpark/battleship reference). All game code reimplemented for Q2.
 * Minesweeper: 9x9 beginner board, safe first reveal, flood-fill uncover,
 * flagging, mine counter and timer (Rockbox apps/plugins/minesweeper.c
 * reference). All game code reimplemented for Q2.
 */
#include "mini_games.h"
static void zero(void *p,unsigned n){unsigned char *b=p;while(n--)*b++=0;}
static uint32_t rnd(Q2Mini *g){
    uint32_t x=g->rng;
    x^=x<<13;x^=x>>17;x^=x<<5;
    return g->rng=x?x:0x12345678u;
}
static void battle_place(Q2Mini *g){
    static const uint8_t size[4]={3,2,2,1};
    for(int id=0;id<4;id++){
        g->ship_length[id]=size[id];
        int placed=0;
        for(int tries=0;tries<500&&!placed;tries++){
            int vertical=(int)(rnd(g)&1u);
            int x=(int)(rnd(g)%6u),y=(int)(rnd(g)%6u);
            int ok=1;
            for(int k=0;k<size[id];k++){
                int xx=x+(vertical?0:k),yy=y+(vertical?k:0);
                if(xx>=6||yy>=6||g->ship_id[yy][xx]){ok=0;break;}
            }
            if(!ok)continue;
            for(int k=0;k<size[id];k++){
                int xx=x+(vertical?0:k),yy=y+(vertical?k:0);
                g->ship_id[yy][xx]=(uint8_t)(id+1);
                g->ships[yy][xx]=1;
            }
            g->target+=size[id];placed=1;
        }
        /* A bounded fallback guarantees fleet completion on any RNG. */
        if(!placed){
            for(int y=0;y<6&&!placed;y++)
                for(int x=0;x<=6-size[id]&&!placed;x++){
                    int ok=1;
                    for(int k=0;k<size[id];k++)if(g->ship_id[y][x+k])ok=0;
                    if(!ok)continue;
                    for(int k=0;k<size[id];k++){
                        g->ship_id[y][x+k]=(uint8_t)(id+1);g->ships[y][x+k]=1;
                    }
                    g->target+=size[id];placed=1;
                }
        }
    }
}
/* Minesweeper: mines are placed on the first reveal, never on or adjacent
 * to the clicked cell, so the first move is always safe. Adjacent counts
 * are computed once at placement; uncovering a zero-count cell flood-fills
 * its neighbours with a bounded explicit stack (81 cells max). */
static int ms_inside(int x,int y){return x>=0&&x<MINI_MS_COLS&&y>=0&&y<MINI_MS_ROWS;}
static void ms_place(Q2Mini *g,int safe_x,int safe_y){
    int placed=0;
    while(placed<g->mine_total){
        int x=(int)(rnd(g)%MINI_MS_COLS),y=(int)(rnd(g)%MINI_MS_ROWS);
        if(g->mines[y][x])continue;
        if(x>=safe_x-1&&x<=safe_x+1&&y>=safe_y-1&&y<=safe_y+1)continue;
        g->mines[y][x]=1;placed++;
    }
    for(int y=0;y<MINI_MS_ROWS;y++)for(int x=0;x<MINI_MS_COLS;x++){
        int n=0;
        for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
            if(!dx&&!dy)continue;
            int nx=x+dx,ny=y+dy;
            if(ms_inside(nx,ny))n+=g->mines[ny][nx];
        }
        g->adj[y][x]=(uint8_t)n;
    }
    g->started=1;
}
static void ms_reveal(Q2Mini *g,int x,int y){
    /* Explicit stack: flood fill never recurses and never exceeds the board. */
    int stack[MINI_MS_ROWS*MINI_MS_COLS][2],top=0;
    stack[top][0]=x;stack[top][1]=y;top++;
    while(top){
        int cx=stack[--top][0],cy=stack[top][1];
        if(!ms_inside(cx,cy)||g->revealed[cy][cx]||g->flagged[cy][cx])continue;
        g->revealed[cy][cx]=1;g->revealed_count++;
        if(g->adj[cy][cx])continue;
        for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
            if(!dx&&!dy)continue;
            if(top<MINI_MS_ROWS*MINI_MS_COLS){
                stack[top][0]=cx+dx;stack[top][1]=cy+dy;top++;
            }
        }
    }
}
static int ms_select(Q2Mini *g){
    int x=g->cursor%MINI_MS_COLS,y=g->cursor/MINI_MS_COLS;
    if(g->revealed[y][x]||g->flagged[y][x])return 0;
    if(!g->started)ms_place(g,x,y);
    g->moves++;
    if(g->mines[y][x]){
        g->revealed[y][x]=1;g->revealed_count++;
        g->finished=1;g->outcome=3;
        return 1;
    }
    ms_reveal(g,x,y);
    if(g->revealed_count==MINI_MS_COLS*MINI_MS_ROWS-g->mine_total){
        g->finished=1;g->won=1;g->outcome=1;
    }
    return 1;
}
void mini_init(Q2Mini *g,int kind,uint32_t seed){
    zero(g,sizeof *g);g->kind=kind;g->rng=seed?seed:1;
    if(kind==MINI_BATTLE){battle_place(g);g->lives=26;}
    if(kind==MINI_MINES){g->mine_total=MINI_MS_MINES;g->cursor=40;}
}
int mini_move(Q2Mini *g,int steps){
    if(g->finished||!steps)return 0;
    int count=g->kind==MINI_BATTLE?36:MINI_MS_COLS*MINI_MS_ROWS;
    int v=(g->cursor+steps)%count;
    g->cursor=v<0?v+count:v;
    return 1;
}
int mini_flag(Q2Mini *g){
    if(g->finished||g->kind!=MINI_MINES)return 0;
    int x=g->cursor%MINI_MS_COLS,y=g->cursor/MINI_MS_COLS;
    if(g->revealed[y][x])return 0;
    if(g->flagged[y][x]){g->flagged[y][x]=0;g->flags_placed--;}
    else{
        if(g->flags_placed>=g->mine_total)return 0;
        g->flagged[y][x]=1;g->flags_placed++;
    }
    return 1;
}
int mini_select(Q2Mini *g){
    if(g->finished)return 0;
    if(g->kind==MINI_BATTLE){
        int x=g->cursor%6,y=g->cursor/6,id=(int)g->ship_id[y][x];
        if(g->board[y][x])return 0;
        g->moves++;
        if(id){
            g->board[y][x]=1;g->score++;
            g->ship_hits[id-1]++;
            g->last_result=g->ship_hits[id-1]==g->ship_length[id-1]?3:1;
        }else{g->board[y][x]=3;g->last_result=2;}
        if(g->score==g->target){g->finished=g->won=1;g->outcome=1;}
        else if(g->moves>=g->lives){g->finished=1;g->outcome=3;}
        return 1;
    }
    if(g->kind==MINI_MINES)return ms_select(g);
    return 0;
}
int mini_tick(Q2Mini *g){
    if(g->kind==MINI_MINES){
        /* One tick per second from the app: run the game timer only once
         * play has started and before the board is finished. */
        if(g->started&&!g->finished)g->elapsed_s++;
        return 1;
    }
    return 0;
}
