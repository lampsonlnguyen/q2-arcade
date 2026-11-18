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
static void test_handheld(void) {
    static const int kinds[] = {ARCADE_TTT, ARCADE_CONNECT, ARCADE_BATTLE,
                                ARCADE_ROAD, ARCADE_CATCH};
    for(unsigned i=0;i<sizeof kinds/sizeof kinds[0];i++) {
        ArcadeApp a;
        app_init(&a,31,0,0);
        for(int n=0;n<kinds[i]-ARCADE_PONG;n++)
            assert(app_input(&a,"w -50"));
        assert(a.selection==kinds[i]-ARCADE_PONG);
        assert(app_input(&a,"k u 218"));
        if(kinds[i]==ARCADE_TTT){
            assert(a.view==VIEW_TTT_SETUP);
            assert(app_input(&a,"t u 180 216"));
        }
        assert(a.game==kinds[i] && a.view==VIEW_PLAY);
        assert(a.mini.kind==kinds[i]-ARCADE_TTT);
        assert(app_input(&a,"w -50"));
        if(kinds[i]<ARCADE_ROAD) {
            assert(app_input(&a,"k u 218"));
            assert(a.mini.moves>0);
        } else {
            assert(app_animated(&a));
            assert(app_tick(&a,200));
        }
        assert(app_input(&a,"k u 170"));
        assert(a.view==VIEW_PAUSE);
        assert(app_input(&a,"k u 170"));
        assert(a.view==VIEW_SELECT);
    }
}
static void test_tic_setup(void){
    for(int size=3;size<=9;size+=3){
        ArcadeApp a;app_init(&a,1,0,0);a.selection=ARCADE_TTT-ARCADE_PONG;
        assert(app_input(&a,"k u 218")&&a.view==VIEW_TTT_SETUP);
        assert(app_input(&a,"k u 170")&&a.view==VIEW_SELECT);
        assert(app_input(&a,"k u 218")&&a.view==VIEW_TTT_SETUP);
        assert(app_input(&a,"t u 180 112")&&a.ttt_pvp==1);
        for(int j=3;j<size;j+=3)assert(app_input(&a,"t u 180 160"));
        assert(app_input(&a,"t u 180 216")&&a.view==VIEW_PLAY);
        assert(a.mini.ttt_size==size&&a.mini.ttt_pvp);
        assert(!app_input(&a,"t u 80 90"));
        assert(app_input(&a,"t u 180 90")&&a.mini.moves==1&&a.mini.ttt_turn==2);
        assert(app_input(&a,"k u 171"));
        assert(app_input(&a,"k u 218")&&a.mini.moves==2&&a.mini.ttt_turn==1);
        assert(app_input(&a,"k u 170")&&a.view==VIEW_PAUSE);
        assert(app_input(&a,"k u 223"));
        assert(app_input(&a,"k u 218")&&a.view==VIEW_PLAY);
        assert(a.mini.moves==0&&a.mini.ttt_size==size&&a.mini.ttt_pvp);
    }
}
int main(void) {
    test_tic_setup();
    test_launcher();test_blocks();test_breakout();test_handheld();
    puts("Q2 Arcade card-app input and game integration tests passed");
    return 0;
}
