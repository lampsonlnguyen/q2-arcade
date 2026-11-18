/* SPDX-License-Identifier: GPL-3.0-or-later
 * Tic-Tac-Toe: full depth-aware minimax (namanyt/tictactoe.c reference).
 * Connect Four: bounded alpha-beta with centre weighting and open-line
 * heuristics (bymayanksingh/connect4 reference).
 * Battleship: randomized non-overlapping ships, hits, misses, sunk status
 * (sehundpark/battleship reference). All game code reimplemented for Q2.
 */
#include "mini_games.h"
static void zero(void *p,unsigned n){unsigned char *b=p;while(n--)*b++=0;}
static uint32_t rnd(Q2Mini *g){
    uint32_t x=g->rng;
    x^=x<<13;x^=x>>17;x^=x<<5;
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
static int ttt_eval(Q2Mini *g,int depth,int cpu){
    if(three(g,2))return 10-depth;
    if(three(g,1))return depth-10;
    int empty=0;
    for(int i=0;i<9;i++)empty+=g->board[i/3][i%3]==0;
    if(!empty)return 0;
    int best=cpu?-100:100;
    /* Prefer centre and corners as a deterministic tie-breaker. */
    static const int order[9]={4,0,2,6,8,1,3,5,7};
    for(int i=0;i<9;i++){
        int p=order[i],r=p/3,c=p%3;
        if(g->board[r][c])continue;
        g->board[r][c]=(uint8_t)(cpu?2:1);
        int score=ttt_eval(g,depth+1,!cpu);
        g->board[r][c]=0;
        if(cpu&&score>best)best=score;
        if(!cpu&&score<best)best=score;
    }
    return best;
}
static void ttt_cpu(Q2Mini *g){
    static const int order[9]={4,0,2,6,8,1,3,5,7};
    int best=-100,pick=-1;
    for(int i=0;i<9;i++){
        int p=order[i],r=p/3,c=p%3;
        if(g->board[r][c])continue;
        g->board[r][c]=2;
        int score=ttt_eval(g,1,0);
        g->board[r][c]=0;
        if(score>best){best=score;pick=p;}
    }
    if(pick>=0){g->board[pick/3][pick%3]=2;g->moves++;}
    if(three(g,2)){g->finished=1;g->outcome=2;}
    else if(g->moves==9){g->finished=1;g->outcome=3;}
}
static int connected(const Q2Mini *g,int c,int r,int mark){
    static const int dir[4][2]={{1,0},{0,1},{1,1},{1,-1}};
    for(int d=0;d<4;d++){
        int count=1;
        for(int sign=-1;sign<=1;sign+=2)
            for(int n=1;n<4;n++){
                int x=c+sign*n*dir[d][0],y=r+sign*n*dir[d][1];
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
static int open_four(const Q2Mini *g,int x,int y,int dx,int dy){
    int cpu=0,human=0;
    for(int k=0;k<4;k++){
        int v=g->board[y+k*dy][x+k*dx];
        cpu+=v==2;human+=v==1;
    }
    if(cpu&&human)return 0;
    if(cpu==4)return 100000;
    if(human==4)return -100000;
    static const int weights[4]={0,2,12,80};
    return cpu?weights[cpu]:human?-weights[human]:0;
}
static int connect_score(const Q2Mini *g){
    int score=0;
    for(int y=0;y<6;y++) {
        score+=(g->board[y][3]==2)*6;
        score-=(g->board[y][3]==1)*6;
    }
    for(int y=0;y<6;y++)for(int x=0;x<=3;x++)score+=open_four(g,x,y,1,0);
    for(int y=0;y<=2;y++)for(int x=0;x<7;x++)score+=open_four(g,x,y,0,1);
    for(int y=0;y<=2;y++)for(int x=0;x<=3;x++)score+=open_four(g,x,y,1,1);
    for(int y=3;y<6;y++)for(int x=0;x<=3;x++)score+=open_four(g,x,y,1,-1);
    return score;
}
static int connect_search(Q2Mini *g,int depth,int cpu,int alpha,int beta){
    if(!depth)return connect_score(g);
    static const int order[7]={3,2,4,1,5,0,6};
    int best=cpu?-200000:200000,found=0;
    for(int i=0;i<7;i++){
        int col=order[i],r=drop(g,col,cpu?2:1);
        if(r<0)continue;
        found=1;
        int value;
        if(connected(g,col,r,cpu?2:1))value=cpu?100000+depth:-100000-depth;
        else value=connect_search(g,depth-1,!cpu,alpha,beta);
        g->board[r][col]=0;
        if(cpu){
            if(value>best)best=value;
            if(best>alpha)alpha=best;
        }else{
            if(value<best)best=value;
            if(best<beta)beta=best;
        }
        if(beta<=alpha)break;
    }
    return found?best:0;
}
static void connect_cpu(Q2Mini *g){
    static const int order[7]={3,2,4,1,5,0,6};
    int best=-200000,pick=-1;
    for(int i=0;i<7;i++){
        int col=order[i],r=drop(g,col,2);
        if(r<0)continue;
        int score=connected(g,col,r,2)?100004:
                  connect_search(g,3,0,-200000,200000);
        g->board[r][col]=0;
        if(score>best){best=score;pick=col;}
    }
    if(pick>=0){
        int r=drop(g,pick,2);
        g->moves++;
        if(connected(g,pick,r,2)){g->finished=1;g->outcome=2;}
        else if(g->moves==42){g->finished=1;g->outcome=3;}
    }
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
void mini_init(Q2Mini *g,int kind,uint32_t seed){
    zero(g,sizeof *g);g->kind=kind;g->rng=seed?seed:1;
    g->lives=3;g->lane=1;g->obstacle_y=-1;
    if(kind==MINI_BATTLE){battle_place(g);g->lives=26;}
    if(kind==MINI_ROAD||kind==MINI_CATCH)g->obstacle=(int)(rnd(g)%3u);
}
int mini_move(Q2Mini *g,int steps){
    if(g->finished||!steps)return 0;
    int count=g->kind==MINI_TTT?9:g->kind==MINI_CONNECT?7:
              g->kind==MINI_BATTLE?36:3;
    if(g->kind>=MINI_ROAD){
        int v=(g->lane+steps)%count;
        if(v<0)v+=count;
        g->lane=v;
    }else{
        int v=(g->cursor+steps)%count;
        if(v<0)v+=count;
        g->cursor=v;
    }
    return 1;
}
int mini_select(Q2Mini *g){
    if(g->finished)return 0;
    if(g->kind==MINI_TTT){
        int row=g->cursor/3,col=g->cursor%3;
        if(g->board[row][col])return 0;
        g->board[row][col]=1;g->moves++;
        if(three(g,1)){g->finished=g->won=1;g->outcome=1;return 1;}
        if(g->moves==9){g->finished=1;g->outcome=3;return 1;}
        ttt_cpu(g);return 1;
    }
    if(g->kind==MINI_CONNECT){
        int r=drop(g,g->cursor,1);
        if(r<0)return 0;
        g->moves++;
        if(connected(g,g->cursor,r,1)){g->finished=g->won=1;g->outcome=1;return 1;}
        if(g->moves==42){g->finished=1;g->outcome=3;return 1;}
        connect_cpu(g);return 1;
    }
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
    return 0;
}
int mini_tick(Q2Mini *g){
    if(g->finished||g->kind<MINI_ROAD)return 0;
    ++g->obstacle_y;
    if(g->obstacle_y==8){
        int collision=g->kind==MINI_ROAD?g->lane==g->obstacle:g->lane!=g->obstacle;
        if(collision&&g->lives)g->lives--;
        if(!collision)g->score++;
        if(!g->lives){g->finished=1;g->outcome=3;}
        g->obstacle_y=-1;
        g->obstacle=(int)(rnd(g)%3u);
    }
    return 1;
}
