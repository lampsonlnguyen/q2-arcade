/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "pool_core.h"
#include <assert.h>
int main(void){
    Q2Pool p;pool_init(&p,42,0);
    for(int i=0;i<16;i++)assert(p.balls[i].live);
    assert(pool_shoot(&p));
    int hit=0;
    for(int i=0;i<3000&&p.moving;i++){pool_step(&p,10);if(p.first>0)hit=1;}
    assert(hit&&!p.moving);
    pool_init(&p,42,1);
    p.ball_in_hand=1;p.balls[0].live=0;
    assert(!pool_place(&p,252,160));
    assert(pool_place(&p,100,160));
    p.balls[0].x=POOL_LEFT*POOL_SCALE;p.balls[0].y=POOL_TOP*POOL_SCALE;
    p.moving=1;p.first=1;pool_step(&p,10);
    assert(p.scratch&&p.ball_in_hand&&p.turn==1);
    pool_init(&p,42,1);
    p.group[0]=1;p.group[1]=2;
    for(int i=1;i<8;i++)p.balls[i].live=0;
    p.balls[8].x=POOL_LEFT*POOL_SCALE;p.balls[8].y=POOL_TOP*POOL_SCALE;
    p.first=8;p.moving=1;pool_step(&p,10);
    assert(p.ended&&p.winner==0);
    pool_init(&p,42,1);p.first=8;p.moving=1;
    p.balls[8].x=POOL_LEFT*POOL_SCALE;p.balls[8].y=POOL_TOP*POOL_SCALE;
    pool_step(&p,10);assert(p.ended&&p.winner==1);
    return 0;
}
