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
    a.selection=ARCADE_BLOCKS-ARCADE_SUDOKU;
    assert(app_input(&a,"k u 218"));
    assert(a.game==ARCADE_BLOCKS && a.view==VIEW_PLAY);
    assert(a.blocks.level==1);
}
static void test_blocks(void) {
    ArcadeApp a;
    app_init(&a, 42, 0, 0);
    a.selection=ARCADE_BLOCKS-ARCADE_SUDOKU;
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
    assert(app_input(&a,"w -56"));
    assert(a.selection==2);
    a.selection=ARCADE_BREAKOUT-ARCADE_SUDOKU;
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
    static const int kinds[] = {ARCADE_BATTLE, ARCADE_MINES};
    for(unsigned i=0;i<sizeof kinds/sizeof kinds[0];i++) {
        ArcadeApp a;
        app_init(&a,31,0,0);
        for(int n=0;n<kinds[i]-ARCADE_SUDOKU;n++)
            assert(app_input(&a,"w -28"));
        assert(a.selection==kinds[i]-ARCADE_SUDOKU);
        assert(app_input(&a,"k u 218"));
        assert(a.game==kinds[i] && a.view==VIEW_PLAY);
        assert(a.mini.kind==kinds[i]-ARCADE_BATTLE);
        assert(app_input(&a,"w -50"));
        if(kinds[i]==ARCADE_BATTLE) {
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
static void test_wheel_sensitivity(void){
    ArcadeApp a;app_init(&a,1,0,0);
    const char config[]="[Q2POD]\nWHEELSENSITIVITY=10\n";
    arcade_theme_parse(&a.theme,config,sizeof config-1);
    for(int i=0;i<139;i++)assert(!app_input(&a,"w -1"));
    assert(app_input(&a,"w -1")&&a.selection==1);
    app_init(&a,1,0,0);a.theme.wheel_percent=100;
    assert(!app_input(&a,"w -13"));
    assert(app_input(&a,"w -1")&&a.selection==1);
    app_init(&a,1,0,0);a.theme.wheel_percent=200;
    assert(app_input(&a,"w -7")&&a.selection==1);
    app_init(&a,1,0,0);a.theme.wheel_percent=160;
    for(int i=0;i<34;i++)(void)app_input(&a,"w -1");
    assert(a.selection==3); /* threshold is 8.75, not rounded to 8 */
    assert(app_input(&a,"w -1")&&a.selection==4);
}
static void test_originals(void){
    for(int i=0;i<ORIGINAL_COUNT;i++){
        ArcadeApp a;app_init(&a,42,0,0);a.selection=i;
        assert(app_input(&a,"k u 218")&&a.game==ARCADE_SUDOKU+i);
        assert(a.runner.status==Q2_GAME_RUNNING);
        assert(app_input(&a,"k u 170")&&a.runner.status==Q2_GAME_PAUSED);
        assert(!app_tick(&a,100));
        assert(app_input(&a,"k u 218")&&a.runner.status==Q2_GAME_RUNNING);
        if(i==ORIGINAL_BRICKS){assert(app_input(&a,"k u 218"));assert(app_animated(&a));assert(app_tick(&a,40));}
        if(i==ORIGINAL_SNAKE){assert(a.original.state.snake.len==3);assert(app_tick(&a,40));}
        if(i==ORIGINAL_SOLITAIRE){assert(a.original.state.sol.len[STOCK]==24);assert(app_input(&a,"k u 218"));assert(a.original.state.sol.len[WASTE]==1);}
        if(i==ORIGINAL_SUDOKU){int n=0;for(int j=0;j<81;j++)n+=!!(a.original.state.sud.cell[j]&CLUE);assert(n>0&&n<81);}
    }
}
int main(void) {
    test_originals();
    ArcadeApp pool;app_init(&pool,42,0,0);
    assert(app_launch(&pool,ARCADE_POOL));
    assert(app_input(&pool,"k d 218"));
    assert(app_tick(&pool,100));
    assert(app_input(&pool,"k u 218")&&pool.pool.moving);
    assert(app_input(&pool,"k u 170")&&pool.runner.status==Q2_GAME_PAUSED);
    assert(!app_tick(&pool,100));
    assert(app_input(&pool,"k u 218")&&pool.runner.status==Q2_GAME_RUNNING);
    assert(ARCADE_GAME_COUNT-ARCADE_SUDOKU==10);
    test_wheel_sensitivity();
    test_launcher();test_blocks();test_breakout();test_handheld();
    puts("Q2 Arcade card-app input and game integration tests passed");
    return 0;
}
