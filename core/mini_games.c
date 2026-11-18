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
    static const int dir[4][2]={{1,0},{0,1},{1,1},{1,-1}};
    int n=g->ttt_size,k=g->ttt_target;
    for(int y=0;y<n;y++)for(int x=0;x<n;x++)for(int d=0;d<4;d++){
        int ex=x+(k-1)*dir[d][0],ey=y+(k-1)*dir[d][1];
        if(ex<0||ex>=n||ey<0||ey>=n)continue;
        int j=0;
        while(j<k&&g->board[y+j*dir[d][1]][x+j*dir[d][0]]==mark)j++;
        if(j==k)return 1;
    }
    return 0;
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
/* Score open winning-length windows through a candidate. Bounded work on 9x9. */
static int ttt_potential(const Q2Mini *g,int x,int y,int mark){
    static const int dir[4][2]={{1,0},{0,1},{1,1},{1,-1}};
    static const int weight[6]={0,2,16,128,1024,8192};
    int score=0,n=g->ttt_size,k=g->ttt_target;
    for(int d=0;d<4;d++)for(int offset=0;offset<k;offset++){
        int sx=x-offset*dir[d][0],sy=y-offset*dir[d][1];
        int ex=sx+(k-1)*dir[d][0],ey=sy+(k-1)*dir[d][1];
        if(sx<0||sx>=n||sy<0||sy>=n||ex<0||ex>=n||ey<0||ey>=n)continue;
        int count=0,blocked=0;
        for(int j=0;j<k;j++){
            int v=g->board[sy+j*dir[d][1]][sx+j*dir[d][0]];
            if(v&&v!=mark){blocked=1;break;}
            count+=v==mark;
        }
        if(!blocked)score+=weight[count+1];
    }
    return score;
}
static int ttt_large_pick(Q2Mini *g){
    int n=g->ttt_size,best=-1,pick=-1;
    for(int y=0;y<n;y++)for(int x=0;x<n;x++){
        if(g->board[y][x])continue;
        g->board[y][x]=2;int win=three(g,2);
        g->board[y][x]=1;int block=three(g,1);
        g->board[y][x]=0;
        int dx=2*x-(n-1),dy=2*y-(n-1);
        int score=win?10000000:block?1000000:
            2*ttt_potential(g,x,y,2)+3*ttt_potential(g,x,y,1)+100-dx*dx-dy*dy;
        if(score>best){best=score;pick=y*n+x;}
    }
    return pick;
}
static void ttt_cpu(Q2Mini *g){
    static const int order[9]={4,0,2,6,8,1,3,5,7};
    int best=-100,pick=-1;
    if(g->ttt_size>3)pick=ttt_large_pick(g);
    else for(int i=0;i<9;i++){
        int p=order[i],r=p/3,c=p%3;
        if(g->board[r][c])continue;
        g->board[r][c]=2;
        int score=ttt_eval(g,1,0);
        g->board[r][c]=0;
        if(score>best){best=score;pick=p;}
    }
    if(pick>=0){g->board[pick/g->ttt_size][pick%g->ttt_size]=2;g->moves++;}
    if(three(g,2)){g->finished=1;g->outcome=2;}
    else if(g->moves==(unsigned)(g->ttt_size*g->ttt_size)){g->finished=1;g->outcome=3;}
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
    g->ttt_size=3;g->ttt_target=3;g->ttt_turn=1;
    g->lives=3;g->lane=1;g->obstacle_y=-1;
    if(kind==MINI_BATTLE){battle_place(g);g->lives=26;}
    if(kind==MINI_ROAD||kind==MINI_CATCH)g->obstacle=(int)(rnd(g)%3u);
}
void mini_ttt_init(Q2Mini *g,int size,int pvp,uint32_t seed){
    mini_init(g,MINI_TTT,seed);
    g->ttt_size=(size==6||size==9)?size:3;
    g->ttt_target=g->ttt_size==3?3:g->ttt_size==6?4:5;
    g->ttt_pvp=!!pvp;
}
int mini_move(Q2Mini *g,int steps){
    if(g->finished||!steps)return 0;
    int count=g->kind==MINI_TTT?g->ttt_size*g->ttt_size:g->kind==MINI_CONNECT?7:
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
        int row=g->cursor/g->ttt_size,col=g->cursor%g->ttt_size;
        if(g->board[row][col])return 0;
        int mark=g->ttt_turn;
        g->board[row][col]=(uint8_t)mark;g->moves++;
        if(three(g,mark)){g->finished=1;g->won=mark==1;g->outcome=mark;return 1;}
        if(g->moves==(unsigned)(g->ttt_size*g->ttt_size)){g->finished=1;g->outcome=3;return 1;}
        if(g->ttt_pvp)g->ttt_turn=3-mark;
        else ttt_cpu(g);
        return 1;
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
