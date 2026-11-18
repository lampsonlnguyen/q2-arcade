/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "mini_games.h"
static void zero(void *p,unsigned n){unsigned char *b=p;while(n--)*b++=0;}
static uint32_t rnd(Q2Mini *g){
    uint32_t x=g->rng; x^=x<<13; x^=x>>17; x^=x<<5;
    return g->rng=x?x:0x12345678u;
}
static int three(const Q2Mini *g,int mark){
    for(int i=0;i<3;i++){
        if(g->board[i][0]==mark&&g->board[i][1]==mark&&g->board[i][2]==mark)return 1;
        if(g->board[0][i]==mark&&g->board[1][i]==mark&&g->board[2][i]==mark)return 1;
    }
    return (g->board[0][0]==mark&&g->board[1][1]==mark&&g->board[2][2]==mark)||
           (g->board[0][2]==mark&&g->board[1][1]==mark&&g->board[2][0]==mark);
}
static int connected(const Q2Mini *g,int c,int r,int mark){
    static const int dirs[4][2]={{1,0},{0,1},{1,1},{1,-1}};
    for(int d=0;d<4;d++){
        int count=1;
        for(int sign=-1;sign<=1;sign+=2)
            for(int n=1;n<4;n++){
                int x=c+sign*n*dirs[d][0],y=r+sign*n*dirs[d][1];
                if(x<0||x>=7||y<0||y>=6||g->board[y][x]!=mark)break;
                ++count;
            }
        if(count>=4)return 1;
    }
    return 0;
}
static int drop(Q2Mini *g,int col,int mark){
    if(col<0||col>=7)return -1;
    for(int row=5;row>=0;row--)if(!g->board[row][col]){
        g->board[row][col]=(uint8_t)mark;return row;
    }
    return -1;
}
static int almost(Q2Mini *g,int mark){
    for(int col=0;col<7;col++) {
        Q2Mini t=*g;int r=drop(&t,col,mark);
        if(r>=0&&connected(&t,col,r,mark))return col;
    }
    return -1;
}
void mini_init(Q2Mini *g,int kind,uint32_t seed){
    zero(g,sizeof *g);g->kind=kind;g->rng=seed?seed:1;
    g->lives=3;g->lane=1;g->obstacle_y=-1;
    if(kind==MINI_BATTLE){
        /* Four nonoverlapping horizontal/vertical ships of lengths 3,2,2,1. */
        static const int sizes[4]={3,2,2,1};
        for(int ship=0;ship<4;ship++)
            for(int tries=0;tries<200;tries++){
                int vertical=(int)(rnd(g)&1u),x=(int)(rnd(g)%6u),y=(int)(rnd(g)%6u);
                int ok=1;
                for(int k=0;k<sizes[ship];k++){
                    int xx=x+(vertical?0:k),yy=y+(vertical?k:0);
                    if(xx>=6||yy>=6||g->ships[yy][xx]){ok=0;break;}
                }
                if(!ok)continue;
                for(int k=0;k<sizes[ship];k++)
                    g->ships[y+(vertical?k:0)][x+(vertical?0:k)]=1;
                g->target+=(unsigned)sizes[ship];break;
            }
        g->lives=18; /* torpedoes; enough for 8 hits with planning. */
    }
    if(kind==MINI_ROAD||kind==MINI_CATCH)g->obstacle=(int)(rnd(g)%3u);
}
int mini_move(Q2Mini *g,int steps){
    if(g->finished||!steps)return 0;
    int old=g->cursor;
    if(g->kind==MINI_TTT){
        int v=(old+steps)%9;if(v<0)v+=9;g->cursor=v;
    }else if(g->kind==MINI_CONNECT){
        int v=(old+steps)%7;if(v<0)v+=7;g->cursor=v;
    }else if(g->kind==MINI_BATTLE){
        int v=(old+steps)%36;if(v<0)v+=36;g->cursor=v;
    }else{
        int v=(g->lane+steps)%3;if(v<0)v+=3;g->lane=v;
    }
    return 1;
}
int mini_select(Q2Mini *g){
    if(g->finished)return 0;
    if(g->kind==MINI_TTT){
        int row=g->cursor/3,col=g->cursor%3;
        if(g->board[row][col])return 0;
        g->board[row][col]=1;g->moves++;
        if(three(g,1)){g->finished=g->won=1;return 1;}
        if(g->moves==9){g->finished=1;return 1;}
        /* Computer wins if possible, blocks threats, then takes centre/corners. */
        int pick=-1;
        for(int pass=0;pass<2&&pick<0;pass++)
            for(int i=0;i<9;i++)if(!g->board[i/3][i%3]){
                Q2Mini t=*g;t.board[i/3][i%3]=(uint8_t)(pass==0?2:1);
                if(three(&t,pass==0?2:1)){pick=i;break;}
            }
        if(pick<0&&!g->board[1][1])pick=4;
        if(pick<0){
            int first=(int)(rnd(g)%9u);
            for(int i=0;i<9;i++){int k=(i+first)%9;if(!g->board[k/3][k%3]){pick=k;break;}}
        }
        if(pick>=0){g->board[pick/3][pick%3]=2;g->moves++;}
        if(three(g,2)||g->moves==9)g->finished=1;
        return 1;
    }
    if(g->kind==MINI_CONNECT){
        int r=drop(g,g->cursor,1);if(r<0)return 0;
        g->moves++;
        if(connected(g,g->cursor,r,1)){g->finished=g->won=1;return 1;}
        if(g->moves==42){g->finished=1;return 1;}
        int col=almost(g,2);if(col<0)col=almost(g,1);
        if(col<0){int first=(int)(rnd(g)%7u);
            for(int k=0;k<7;k++){int c=(first+k)%7;
                if(g->board[0][c]==0){col=c;break;}}
        }
        r=drop(g,col,2);if(r>=0){g->moves++;
            if(connected(g,col,r,2)||g->moves==42)g->finished=1;
        }
        return 1;
    }
    if(g->kind==MINI_BATTLE){
        int x=g->cursor%6,y=g->cursor/6;
        if(g->board[y][x])return 0;
        int hit=g->ships[y][x];
        g->board[y][x]=(uint8_t)(hit?1:3);
        if(hit)g->score++;
        g->moves++;
        if(g->score==g->target){g->finished=g->won=1;}
        else if(g->moves>=18)g->finished=1;
        return 1;
    }
    return 0;
}
int mini_tick(Q2Mini *g){
    if(g->finished||g->kind<MINI_ROAD)return 0;
    ++g->obstacle_y;
    if(g->obstacle_y==8){
        int collision=(g->kind==MINI_ROAD ? g->lane==g->obstacle :
                       g->lane!=g->obstacle);
        if(collision&&g->lives)g->lives--;
        if(!collision)g->score++;
        if(!g->lives)g->finished=1;
        g->obstacle_y=-1;g->obstacle=(int)(rnd(g)%3u);
    }
    return 1;
}
