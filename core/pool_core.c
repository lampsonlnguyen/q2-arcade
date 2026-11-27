/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "pool_core.h"
#include "original_games.h"
#define S POOL_SCALE
#define R (POOL_RADIUS*S)
static int abs_i(int x){return x<0?-x:x;}
static int clamp(int x,int lo,int hi){return x<lo?lo:x>hi?hi:x;}
static unsigned root(unsigned n){
    unsigned bit=1u<<30,r=0;
    while(bit>n)bit>>=2;
    while(bit){if(n>=r+bit){n-=r+bit;r=(r>>1)+bit;}else r>>=1;bit>>=2;}
    return r;
}
static unsigned random_next(Q2Pool *p){return p->seed=p->seed*1664525u+1013904223u;}
static int group(int i){return i<8?1:2;}
static int remaining(const Q2Pool *p,int g){
    int n=0;for(int i=1;i<16;i++)if(i!=8&&p->balls[i].live&&group(i)==g)n++;
    return n;
}
void pool_init(Q2Pool *p,unsigned seed,int local){
    *p=(Q2Pool){0};p->seed=seed?seed:1;p->local=local;p->power=65;p->wheel_percent=50;
    p->winner=-1;p->first=-1;
    p->balls[0]=(PoolBall){100*S,160*S,0,0,1};
    static const int order[]={1,9,2,10,8,3,4,11,5,12,13,6,14,7,15};
    int k=0;
    for(int col=0;col<5;col++)for(int row=0;row<=col;row++){
        int i=order[k++];
        p->balls[i]=(PoolBall){(252+col*11)*S,(160+row*13)*S-col*13*S/2,0,0,1};
    }
}
int pool_place(Q2Pool *p,int x,int y){
    if(!p->ball_in_hand||p->moving||p->ended)return 0;
    x=clamp(x,POOL_LEFT+POOL_RADIUS,POOL_RIGHT-POOL_RADIUS)*S;
    y=clamp(y,POOL_TOP+POOL_RADIUS,POOL_BOTTOM-POOL_RADIUS)*S;
    for(int i=1;i<16;i++)if(p->balls[i].live){
        int dx=(x-p->balls[i].x)/16,dy=(y-p->balls[i].y)/16;
        if(dx*dx+dy*dy<4*(R/16)*(R/16))return 0;
    }
    p->balls[0]=(PoolBall){x,y,0,0,1};p->ball_in_hand=0;return 1;
}
int pool_shoot(Q2Pool *p){
    if(p->moving||p->ended||p->ball_in_hand||!p->balls[0].live)return 0;
    p->angle&=1023;
    int speed=150+p->power*15;
    p->balls[0].vx=(original_cos(p->angle)/256)*speed/256;
    p->balls[0].vy=(original_sin(p->angle)/256)*speed/256;
    p->moving=1;p->charging=0;p->first=-1;p->potted=0;p->mask=0;p->scratch=0;p->rail=0;return 1;
}
static void pocket(Q2Pool *p,int i){
    PoolBall *b=&p->balls[i];b->live=0;b->vx=b->vy=0;
    if(!i)p->scratch=1;
    else{p->potted++;p->mask|=1<<i;}
}
static int near_pocket(int x,int y){
    static const int px[]={POOL_LEFT,187,POOL_RIGHT,POOL_LEFT,187,POOL_RIGHT};
    static const int py[]={POOL_TOP,POOL_TOP,POOL_TOP,POOL_BOTTOM,POOL_BOTTOM,POOL_BOTTOM};
    for(int i=0;i<6;i++){
        int dx=(x-px[i]*S)/16,dy=(y-py[i]*S)/16;
        if(dx*dx+dy*dy<(11*S/16)*(11*S/16))return 1;
    }
    return 0;
}
static void collide(Q2Pool *p,int i,int j){
    PoolBall *a=&p->balls[i],*b=&p->balls[j];
    int dx=(b->x-a->x)/8,dy=(b->y-a->y)/8;
    if(abs_i(dx)>=2*R/8||abs_i(dy)>=2*R/8)return;
    unsigned d2=(unsigned)(dx*dx+dy*dy);
    if(d2>=(unsigned)(2*R/8)*(2*R/8))return;
    int dist=(int)root(d2);
    if(!dist){dx=1;dy=0;dist=1;}
    int nx=dx*256/dist,ny=dy*256/dist;
    int overlap=2*R-dist*8;
    a->x-=nx*overlap/512;b->x+=nx*overlap/512;
    a->y-=ny*overlap/512;b->y+=ny*overlap/512;
    int impulse=((a->vx-b->vx)*nx+(a->vy-b->vy)*ny)/256;
    if(impulse<=0)return;
    if(i==0&&p->first<0)p->first=j;
    impulse=impulse*95/100;
    a->vx-=impulse*nx/256;a->vy-=impulse*ny/256;
    b->vx+=impulse*nx/256;b->vy+=impulse*ny/256;
}
static void end_shot(Q2Pool *p){
    int g=p->group[p->turn],own=0;
    for(int i=1;i<16;i++)if(i!=8&&(p->mask&(1<<i))&&group(i)==g)own++;
    int before= g?remaining(p,g)+own:7;
    int foul=p->scratch||p->first<0||(!p->potted&&!p->rail)||
        (p->first==8&&before>0)||(g&&p->first!=8&&group(p->first)!=g);
    p->moving=0;
    if(p->mask&(1<<8)){
        p->ended=1;p->winner=(!foul&&g&&before==0)?p->turn:1-p->turn;return;
    }
    if(!g&&!foul&&p->potted){
        for(int i=1;i<16;i++)if(p->mask&(1<<i)){
            g=group(i);p->group[p->turn]=g;p->group[1-p->turn]=3-g;break;
        }
        own=0;for(int i=1;i<16;i++)if(i!=8&&(p->mask&(1<<i))&&group(i)==g)own++;
    }
    if(foul||!own)p->turn=1-p->turn;
    if(foul){p->ball_in_hand=1;p->balls[0].live=0;}
    p->delay=65;
}
static void ai(Q2Pool *p){
    if(p->ball_in_hand){
        for(int x=70;x<=310&&p->ball_in_hand;x+=20)
            for(int y=90;y<=230&&p->ball_in_hand;y+=20)(void)pool_place(p,x,y);
        if(p->ball_in_hand)return;
    }
    int g=p->group[p->turn],target=-1,best=2147483647;
    static const int px[]={POOL_LEFT,187,POOL_RIGHT,POOL_LEFT,187,POOL_RIGHT};
    static const int py[]={POOL_TOP,POOL_TOP,POOL_TOP,POOL_BOTTOM,POOL_BOTTOM,POOL_BOTTOM};
    int ax=0,ay=0;
    for(int i=1;i<16;i++)if(p->balls[i].live&&(i==8?(g&&!remaining(p,g)):(!g||group(i)==g))){
        int bx=p->balls[i].x/S,by=p->balls[i].y/S;
        for(int k=0;k<6;k++){
            int dx=px[k]-bx,dy=py[k]-by,dist=(int)root((unsigned)(dx*dx+dy*dy));
            if(!dist)continue;
            int tx=bx-dx*12/dist,ty=by-dy*12/dist;
            if(tx<POOL_LEFT+6||tx>POOL_RIGHT-6||ty<POOL_TOP+6||ty>POOL_BOTTOM-6)continue;
            int cx=tx-p->balls[0].x/S,cy=ty-p->balls[0].y/S;
            if(cx*dx+cy*dy<=0)continue;
            int score=cx*cx+cy*cy+dist*dist/2;
            if(score<best){best=score;target=i;ax=cx;ay=cy;}
        }
    }
    if(target<0)for(int i=1;i<16;i++)if(p->balls[i].live&&i!=8&&(!g||group(i)==g)){target=i;break;}
    if(target<0&&p->balls[8].live)target=8;
    if(target<0)return;
    if(!ax&&!ay){ax=(p->balls[target].x-p->balls[0].x)/S;ay=(p->balls[target].y-p->balls[0].y)/S;}
    p->angle=(original_angle(ay,ax)+(int)(random_next(p)%9)-4)&1023;
    p->power=70+(int)(random_next(p)%21);pool_shoot(p);
}
void pool_step(Q2Pool *p,unsigned ms){
    (void)ms; /* runner supplies a fixed 10 ms step */
    if(p->ended)return;
    if(!p->moving){
        if(p->charging&&p->power<100)p->power++;
        if(!p->local&&p->turn==1){if(p->delay>0)p->delay--;else ai(p);}
        return;
    }
    for(int i=0;i<16;i++)if(p->balls[i].live){
        PoolBall *b=&p->balls[i];b->x+=b->vx;b->y+=b->vy;
        if(near_pocket(b->x,b->y)){pocket(p,i);continue;}
        if(b->x<POOL_LEFT*S+R||b->x>POOL_RIGHT*S-R){
            b->x=clamp(b->x,POOL_LEFT*S+R,POOL_RIGHT*S-R);b->vx=-b->vx*82/100;if(p->first>=0)p->rail=1;
        }
        if(b->y<POOL_TOP*S+R||b->y>POOL_BOTTOM*S-R){
            b->y=clamp(b->y,POOL_TOP*S+R,POOL_BOTTOM*S-R);b->vy=-b->vy*82/100;if(p->first>=0)p->rail=1;
        }
    }
    for(int pass=0;pass<2;pass++)for(int i=0;i<16;i++)if(p->balls[i].live)
        for(int j=i+1;j<16;j++)if(p->balls[j].live)collide(p,i,j);
    int moving=0;
    for(int i=0;i<16;i++)if(p->balls[i].live){
        PoolBall *b=&p->balls[i];b->vx=b->vx*252/256;b->vy=b->vy*252/256;
        if(abs_i(b->vx)<7)b->vx=0;
        if(abs_i(b->vy)<7)b->vy=0;
        moving|=b->vx||b->vy;
    }
    if(!moving)end_shot(p);
}
int pool_animated(const Q2Pool *p){return !p->ended&&(p->moving||p->charging||(!p->local&&p->turn==1));}
int pool_input(Q2Pool *p,Q2GameInput e){
    if(p->ended||p->moving||(!p->local&&p->turn==1))return 0;
    if(e.action==Q2_GAME_PLAY){if(p->ball_in_hand)return pool_place(p,100,160);p->power=p->power>=100?25:p->power+15;return 1;}
    if(e.action==Q2_GAME_TOUCH_UP){
        if(p->ball_in_hand)return pool_place(p,e.x,e.y);
        if(e.y>=280){p->power=clamp((e.x-60)*100/155,15,100);return 1;}
        p->angle=original_angle(e.y-p->balls[0].y/S,e.x-p->balls[0].x/S)&1023;return 1;
    }
    if(p->ball_in_hand)return 0;
    if(e.action==Q2_GAME_WHEEL){
        p->wheel_acc-=e.value*p->wheel_percent;
        p->angle=(p->angle+p->wheel_acc/50)&1023;p->wheel_acc%=50;return 1;
    }
    if(e.action==Q2_GAME_PREV||e.action==Q2_GAME_NEXT){p->angle=(p->angle+(e.action==Q2_GAME_PREV?-2:2))&1023;return 1;}
    if(e.action==Q2_GAME_CENTER)return pool_shoot(p);
    return 0;
}
static int hook_input(void *c,Q2GameInput e){return pool_input(c,e);}
static void hook_step(void *c,unsigned ms){pool_step(c,ms);}
static void hook_reset(void *c){Q2Pool *p=c;unsigned seed=p->seed;int local=p->local,pct=p->wheel_percent;pool_init(p,seed,local);p->wheel_percent=pct;}
const Q2GameHooks pool_hooks={hook_input,hook_step,0,hook_reset};
