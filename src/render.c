/* SPDX-License-Identifier: GPL-3.0-or-later
 * Q2 framebuffer renderer: logical 375x320 landscape, portrait stride.
 * Simple integer rectangles/5x7 font; no image/font assets or system UI.
 */
#include "app.h"

enum { BG=0x10141c, PANEL=0x19232f, OUTLINE=0x526074,
       WHITE=0xf0f2f5, DIM=0x98a7b9, CYAN=0x4ad3d6,
       SELECT=0x396078, GOLD=0xf9ce58, BLACK=0x0b1018 };
static const u32 palette[] = {
    BG, 0x32cbe6, 0xe8cd47, 0xaf80e7, 0x4bc785,
    0xe86666, 0x578bdf, 0xf39b40
};
static const unsigned char letters[26][7] = {
    {14,17,17,31,17,17,17}, {30,17,17,30,17,17,30},
    {14,17,16,16,16,17,14}, {30,17,17,17,17,17,30},
    {31,16,16,30,16,16,31}, {31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15}, {17,17,17,31,17,17,17},
    {14,4,4,4,4,4,14}, {1,1,1,1,17,17,14},
    {17,18,20,24,20,18,17}, {16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17}, {17,25,21,19,17,17,17},
    {14,17,17,17,17,17,14}, {30,17,17,30,16,16,16},
    {14,17,17,17,21,18,13}, {30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30}, {31,4,4,4,4,4,4},
    {17,17,17,17,17,17,14}, {17,17,17,17,17,10,4},
    {17,17,17,21,21,21,10}, {17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4}, {31,1,2,4,8,16,31}
};
static const unsigned char digits[10][7] = {
    {14,17,19,21,25,17,14}, {4,12,4,4,4,4,14},
    {14,17,1,2,4,8,31}, {30,1,1,14,1,1,30},
    {2,6,10,18,31,2,2}, {31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14}, {31,1,2,4,8,8,8},
    {14,17,17,14,17,17,14}, {14,17,17,15,1,1,14}
};
static void fill(u32 *dst, unsigned stride, int x, int y, int w, int h, u32 color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;
    color |= 0xff000000u;
    for (int xx = x; xx < x + w; ++xx) {
        u32 *row = dst + xx * stride;
        for (int yy = y; yy < y + h; ++yy)
            row[SCREEN_H - 1 - yy] = color;
    }
}
static void border(u32 *d, unsigned s, int x, int y, int w, int h, u32 c) {
    fill(d,s,x,y,w,1,c); fill(d,s,x,y+h-1,w,1,c);
    fill(d,s,x,y,1,h,c); fill(d,s,x+w-1,y,1,h,c);
}
static const unsigned char *glyph(char c) {
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c >= 'A' && c <= 'Z') return letters[c - 'A'];
    if (c >= '0' && c <= '9') return digits[c - '0'];
    return 0;
}
static void text(u32 *d, unsigned s, int x, int y, const char *str, u32 color, int scale) {
    for (; *str; ++str) {
        const unsigned char *g = glyph(*str);
        if (g) {
            for (int row = 0; row < 7; ++row)
                for (int col = 0; col < 5; ++col)
                    if (g[row] & (16u >> col))
                        fill(d,s,x+col*scale,y+row*scale,scale,scale,color);
        } else if (*str == '-' || *str == ':') {
            int line = *str == '-' ? 3 : 1;
            fill(d,s,x+scale,y+line*scale,3*scale,scale,color);
            if (*str == ':') fill(d,s,x+scale,y+5*scale,3*scale,scale,color);
        } else if (*str == '/') {
            for (int row = 0; row < 7; ++row)
                fill(d,s,x+(5-row*5/7)*scale,y+row*scale,scale,scale,color);
        } else if (*str == '.') {
            fill(d,s,x+2*scale,y+6*scale,scale,scale,color);
        } else if (*str == '<') {
            fill(d,s,x+1*scale,y+3*scale,scale,scale,color);
            fill(d,s,x+2*scale,y+2*scale,scale,scale,color);
            fill(d,s,x+2*scale,y+4*scale,scale,scale,color);
        } else if (*str == '>') {
            fill(d,s,x+3*scale,y+3*scale,scale,scale,color);
            fill(d,s,x+2*scale,y+2*scale,scale,scale,color);
            fill(d,s,x+2*scale,y+4*scale,scale,scale,color);
        }
        x += 6*scale;
    }
}
static void centered(u32 *d,unsigned s,int y,const char *str,u32 c,int scale) {
    int n=0; while(str[n]) ++n;
    text(d,s,(SCREEN_W-n*6*scale)/2,y,str,c,scale);
}
static void number(u32 *d, unsigned s, int x, int y, unsigned value, u32 c, int scale) {
    char out[12]; int n=0;
    do { out[n++] = (char)('0'+value%10); value/=10; } while(value && n<10);
    for (int i=0; i<n/2; ++i) { char t=out[i]; out[i]=out[n-i-1]; out[n-i-1]=t; }
    out[n]=0; text(d,s,x,y,out,c,scale);
}
static void line_heading(u32*d,unsigned s,const char*name) {
    fill(d,s,0,0,SCREEN_W,47,BG);
    centered(d,s,18,name,WHITE,2);
    fill(d,s,20,47,335,1,OUTLINE);
}
static void options(u32*d,unsigned s,const char*heading,const char*const*names,int count,int selected) {
    fill(d,s,57,75,261,229,BLACK);
    border(d,s,57,75,261,229,OUTLINE);
    centered(d,s,89,heading,WHITE,2);
    int spacing=count==4?43:count==3?57:72;
    for(int i=0;i<count;++i){
        int y=123+i*spacing;
        fill(d,s,77,y,221,spacing-8,i==selected?SELECT:PANEL);
        text(d,s,91,y+11,names[i],i==selected?WHITE:DIM,2);
    }
}
static void menu(u32*d,unsigned s,int selection) {
    static const char* names[]={"PONG","BLOCKS","BREAKOUT"};
    line_heading(d,s,"Q2 ARCADE");
    centered(d,s,69,"SELECT A GAME",DIM,2);
    for(int i=0;i<3;++i){
        int y=86+i*58;
        fill(d,s,29,y,317,49,i==selection?SELECT:PANEL);
        border(d,s,29,y,317,49,i==selection?CYAN:OUTLINE);
        text(d,s,49,y+16,names[i],WHITE,2);
        text(d,s,315,y+16,">",GOLD,2);
    }
    centered(d,s,282,"WHEEL SELECT  CENTER OPEN",DIM,1);
}
static void pong(u32*d,unsigned s,const ArcadeApp *a) {
    const Q2Pong*p=&a->pong;
    line_heading(d,s,"PONG");
    text(d,s,30,52,"YOU",DIM,1); number(d,s,62,52,p->player_score,WHITE,1);
    text(d,s,242,52,"CPU",DIM,1); number(d,s,274,52,p->cpu_score,WHITE,1);
    fill(d,s,27,68,321,217,BLACK);
    border(d,s,27,68,321,217,OUTLINE);
    for(int y=77;y<270;y+=22)fill(d,s,187,y,2,12,OUTLINE);
    fill(d,s,34,68+p->player_y*214/240,8,48*214/240,CYAN);
    fill(d,s,333,68+p->cpu_y*214/240,8,48*214/240,WHITE);
    fill(d,s,27+p->ball_x,68+p->ball_y*214/240,7,7,GOLD);
    centered(d,s,296,"WHEEL MOVE  RETURN PAUSE",DIM,1);
}
static void blocks(u32*d,unsigned s,const ArcadeApp*a) {
    const Q2Blocks*b=&a->blocks;
    line_heading(d,s,"BLOCKS");
    text(d,s,28,63,"SCORE",DIM,1);
    number(d,s,28,79,b->score,WHITE,2);
    text(d,s,258,63,"LEVEL",DIM,1);
    number(d,s,272,80,b->level,WHITE,2);
    fill(d,s,102,66,112,222,BLACK);
    border(d,s,101,65,114,224,OUTLINE);
    for(int y=0;y<Q2_BOARD_H;++y)
        for(int x=0;x<Q2_BOARD_W;++x){
            int cell=q2_blocks_cell(b,x,y);
            fill(d,s,103+x*11,67+y*11,10,10,
                 cell>0&&cell<8?palette[cell]:PANEL);
        }
    text(d,s,253,130,"NEXT",DIM,2);
    for(int y=0;y<4;++y)
        for(int x=0;x<4;++x){
            int cell=q2_blocks_next_cell(b,x,y);
            if(cell>0&&cell<8)
                fill(d,s,254+x*13,162+y*13,12,12,palette[cell]);
        }
    text(d,s,247,235,"BEST",DIM,1);
    number(d,s,247,248,a->best_blocks,WHITE,1);
    text(d,s,13,297,"WHEEL ROTATE  LEFT RIGHT MOVE",DIM,1);
}
static void breakout(u32*d,unsigned s,const ArcadeApp*a) {
    const Q2Breakout*b=&a->breakout;
    line_heading(d,s,"BREAKOUT");
    text(d,s,29,52,"SCORE",DIM,1);
    number(d,s,77,52,b->score,WHITE,1);
    text(d,s,200,52,"LIVES",DIM,1);
    number(d,s,248,52,b->lives,WHITE,1);
    text(d,s,283,52,"LV",DIM,1);
    number(d,s,308,52,b->level,WHITE,1);
    fill(d,s,27,68,321,217,BLACK);
    border(d,s,27,68,321,217,OUTLINE);
    for(int y=0;y<5;++y)
        for(int x=0;x<8;++x){
            int cell=b->bricks[y][x];
            if(cell) fill(d,s,43+x*36,68+(27+y*16)*214/240,32,12,palette[cell]);
        }
    fill(d,s,27+b->paddle_x,68+220*214/240,48,6,CYAN);
    fill(d,s,27+b->ball_x-3,68+b->ball_y*214/240-3,7,7,WHITE);
    if(b->waiting)centered(d,s,215,"CENTER TO LAUNCH",GOLD,1);
    centered(d,s,296,"WHEEL MOVE  RETURN PAUSE",DIM,1);
}
void app_render(const ArcadeApp *a, u32 *d, unsigned s) {
    fill(d,s,0,0,SCREEN_W,SCREEN_H,BG);
    if(a->view==VIEW_SELECT){menu(d,s,a->selection);return;}
    if(a->game==ARCADE_PONG)pong(d,s,a);
    else if(a->game==ARCADE_BLOCKS)blocks(d,s,a);
    else if(a->game==ARCADE_BREAKOUT)breakout(d,s,a);
    if(a->view==VIEW_PAUSE) {
        static const char* names[]={"RESUME","RESTART","GAMES","EXIT"};
        options(d,s,"PAUSED",names,4,a->option);
    } else if(a->view==VIEW_OVER) {
        static const char* names[]={"RESTART","GAMES"};
        options(d,s,"GAME OVER",names,2,a->option);
    }
}
