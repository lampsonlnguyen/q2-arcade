/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef Q2_POOL_CORE_H
#define Q2_POOL_CORE_H
#include "game_framework.h"
enum {POOL_SCALE=256,POOL_LEFT=35,POOL_RIGHT=340,POOL_TOP=66,POOL_BOTTOM=254,POOL_RADIUS=6};
typedef struct {int x,y,vx,vy,live;} PoolBall;
typedef struct {
    PoolBall balls[16];
    int angle,power,charging,moving,turn,group[2],winner,ended,local;
    int first,potted,mask,scratch,rail,ball_in_hand,delay,wheel_percent,wheel_acc;
    unsigned seed;
} Q2Pool;
void pool_init(Q2Pool *p,unsigned seed,int local);
int pool_shoot(Q2Pool *p);
void pool_step(Q2Pool *p,unsigned ms);
int pool_input(Q2Pool *p,Q2GameInput e);
int pool_place(Q2Pool *p,int x,int y);
int pool_animated(const Q2Pool *p);
extern const Q2GameHooks pool_hooks;
#endif
