/* Host-only integration tests for the downloadable card app's input contract. */
#include "../src/app.h"
#include <assert.h>
#include <stdio.h>

static void test_launcher(void) {
    ArcadeApp a;
    app_init(&a, 42, 321, 123);
    assert(a.view==VIEW_SELECT && a.best_blocks==321);
    assert(app_input(&a,"w -50"));
    assert(a.selection==1);
    assert(app_input(&a,"k u 218"));
    assert(a.game==ARCADE_BLOCKS && a.view==VIEW_PLAY);
    assert(a.blocks.level==1);
}
static void test_blocks(void) {
    ArcadeApp a;
    app_init(&a, 42, 0, 0);
    assert(app_input(&a,"w -50"));
    assert(app_input(&a,"k u 218"));
    int x=a.blocks.x;
    assert(app_input(&a,"k u 223"));
    assert(a.blocks.x==x+1);
    assert(app_input(&a,"k u 222"));
    assert(a.blocks.x==x);
    unsigned rotation=a.blocks.rotation;
    /* Small wheel movement should accumulate, not rotate immediately. */
    assert(!app_input(&a,"w -15"));
    assert(!app_input(&a,"w -15"));
    assert(app_input(&a,"w -20"));
    assert(a.blocks.rotation==(rotation+1)%4);
    assert(app_input(&a,"w 50"));
    assert(a.blocks.rotation==rotation);
    /* Large bursts cannot rotate a piece multiple times per packet. */
    assert(app_input(&a,"w -160"));
    assert(a.blocks.rotation==(rotation+1)%4);
    assert(app_input(&a,"w 50"));
    assert(a.blocks.rotation==rotation);
    int y=a.blocks.y;
    assert(app_input(&a,"k u 171"));
    assert(a.blocks.y>y && a.view==VIEW_PLAY);
    assert(app_input(&a,"k u 218"));
    assert(a.blocks.score>0);
    assert(app_input(&a,"k u 170"));
    assert(a.view==VIEW_PAUSE);
    assert(app_input(&a,"k u 218"));
    assert(a.view==VIEW_PLAY);
    assert(app_input(&a,"k u 170"));
    assert(app_input(&a,"k u 170"));
    assert(a.view==VIEW_SELECT);
}
static void test_breakout(void) {
    ArcadeApp a;
    app_init(&a, 11, 0, 0);
    assert(app_input(&a,"w -100"));
    assert(a.selection==2);
    assert(app_input(&a,"k u 218"));
    assert(a.breakout.remaining==40 && a.breakout.waiting);
    assert(app_input(&a,"k u 218"));
    assert(!a.breakout.waiting);
    assert(app_tick(&a,40));
    assert(a.breakout.ball_y<214);
    assert(app_input(&a,"k u 170"));
    assert(a.view==VIEW_PAUSE && !app_animated(&a));
}
int main(void) {
    test_launcher();test_blocks();test_breakout();
    puts("Q2 Arcade card-app input and game integration tests passed");
    return 0;
}
