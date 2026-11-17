/* SPDX-License-Identifier: GPL-3.0-or-later
 * Q2 framebuffer renderer: logical 375x320 landscape, portrait stride.
 * Simple integer rectangles/5x7 font; no image/font assets or system UI.
 */
#include "app.h"

/* Q2 Pod assets/themes and patch/offsets.inc supply the theme IDs and accents.
 * Read saved settings once at launch; keep the game-piece colors distinct. */
static u32 BG=0, PANEL=0x191919, OUTLINE=0x444444,
           WHITE=0xeeeeec, DIM=0x949294, CYAN=0xeeeeec,
           SELECT=0xeeeeec, GOLD=0xeeeeec, BLACK=0, selected_text=0x161616;
static int config_number(const char *config, const char *key, int fallback, int limit) {
    int ipod=0;
    while (*config) {
        const char *end=config;
        while (*end && *end!='\n') ++end;
        const char *p=config;
        while (p<end && (*p==' ' || *p=='\t')) ++p;
        if (p<end && *p=='[') {
            const char section[]="[IPOD]";
            int i=0;
            while (p+i<end && section[i] && p[i]==section[i]) ++i;
            ipod=i==6;
        } else if (ipod) {
            int i=0;
            while (p+i<end && key[i] && p[i]==key[i]) ++i;
            if (!key[i]) {
                p+=i;
                while (p<end && (*p==' ' || *p=='\t')) ++p;
                if (p<end && *p++=='=') {
                    while (p<end && (*p==' ' || *p=='\t')) ++p;
                    unsigned value=0; int digits=0;
                    while (p<end && *p>='0' && *p<='9') {
                        value=value*10+(unsigned)(*p++-'0');
                        if (value>=(unsigned)limit) return fallback;
                        digits=1;
                    }
                    if (digits && (p==end || *p=='\r' || *p==' ' || *p=='\t' || *p==';' || *p=='#'))
                        return (int)value;
                }
            }
        }
        config=*end?end+1:end;
    }
    return fallback;
}
void app_theme(const char *config, int screen_light) {
    static const u32 accents[]={0x424242,0xe8123f,0x13838d,0x8c732c,
        0x2a6ad8,0xc45400,0x903868,0x424242,0x647f34};
    int theme=config_number(config,"THEME",0,3);
    const char *key=theme==0?"MINIMAL_ACCENT":theme==1?"CLASSIC_LIGHT_ACCENT":"ACCENT";
    int accent=config_number(config,key,0,9);
    if (theme==1) {
        /* Classic Blue precedes Graphite; Classic has no Ivory. */
        accent=accent==0?4:accent==8?8:accent-1;
    } else if (theme==2 && accent==7) accent=0;
    int mode=config_number(config,"LIGHT",0,3);
    int light=mode==1 || (mode==2 && screen_light);
    BG=light?(theme==1?0xffffff:0xf4f4f4):0;
    BLACK=BG; PANEL=light?0xe7e7e7:0x191919;
    OUTLINE=light?0xb0b0b0:0x444444;
    WHITE=light?(theme==1?0:0x161616):0xeeeeec;
    DIM=light?0x606060:0x949294;
    CYAN=GOLD=SELECT=accents[accent]; selected_text=0xffffff;
    if (theme==0 && (accent==0 || accent==7)) {
        SELECT=light?0x161616:0xeeeeec;
        CYAN=GOLD=WHITE; selected_text=BG;
    }
}
static const u32 palette[] = {
    0, 0x32cbe6, 0xe8cd47, 0xaf80e7, 0x4bc785,
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
/* HUD and labels use the shared framework's safe-area geometry. This is
 * intentionally only for UI/text: background and gameplay drawing remain
 * full-frame so a rounded corner never changes gameplay coordinates. */
static void ui_pixel(u32 *d, unsigned s, int x, int y, int scale, u32 color) {
    for (int yy = 0; yy < scale; ++yy)
        for (int xx = 0; xx < scale; ++xx)
            if (q2_game_ui_safe_point(SCREEN_W, SCREEN_H, x + xx, y + yy))
                fill(d, s, x + xx, y + yy, 1, 1, color);
}
static const unsigned char *glyph(char c) {
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c >= 'A' && c <= 'Z') return letters[c - 'A'];
    if (c >= '0' && c <= '9') return digits[c - '0'];
    return 0;
}
static void text(u32 *d, unsigned s, int x, int y, const char *str, u32 color, int scale) {
    int step=scale==3?3:scale*2;
    if(scale==3)scale=1;
    for (; *str; ++str) {
        const unsigned char *g = glyph(*str);
        if (g) {
            for (int row = 0; row < 7; ++row)
                for (int col = 0; col < 5; ++col)
                    if (g[row] & (16u >> col))
                        ui_pixel(d,s,x+col*scale,y+row*scale,scale,color);
        } else if (*str == '-' || *str == ':') {
            int line = *str == '-' ? 3 : 1;
            ui_pixel(d,s,x+scale,y+line*scale,scale,color), ui_pixel(d,s,x+2*scale,y+line*scale,scale,color), ui_pixel(d,s,x+3*scale,y+line*scale,scale,color);
            if (*str == ':') ui_pixel(d,s,x+scale,y+5*scale,scale,color), ui_pixel(d,s,x+2*scale,y+5*scale,scale,color), ui_pixel(d,s,x+3*scale,y+5*scale,scale,color);
        } else if (*str == '/') {
            for (int row = 0; row < 7; ++row)
                ui_pixel(d,s,x+(5-row*5/7)*scale,y+row*scale,scale,color);
        } else if (*str == '.') {
            ui_pixel(d,s,x+2*scale,y+6*scale,scale,color);
        } else if (*str == '<') {
            ui_pixel(d,s,x+1*scale,y+3*scale,scale,color);
            ui_pixel(d,s,x+2*scale,y+2*scale,scale,color);
            ui_pixel(d,s,x+2*scale,y+4*scale,scale,color);
        } else if (*str == '>') {
            ui_pixel(d,s,x+3*scale,y+3*scale,scale,color);
            ui_pixel(d,s,x+2*scale,y+2*scale,scale,color);
            ui_pixel(d,s,x+2*scale,y+4*scale,scale,color);
        }
        x += 3*step;
    }
}
static void centered(u32 *d,unsigned s,int y,const char *str,u32 c,int scale) {
    int n=0; while(str[n]) ++n;
    text(d,s,(SCREEN_W-n*(scale==3?9:6*scale))/2,y,str,c,scale);
}
static void number(u32 *d, unsigned s, int x, int y, unsigned value, u32 c, int scale) {
    char out[12]; int n=0;
    do { out[n++] = (char)('0'+value%10); value/=10; } while(value && n<10);
    for (int i=0; i<n/2; ++i) { char t=out[i]; out[i]=out[n-i-1]; out[n-i-1]=t; }
    out[n]=0; text(d,s,x,y,out,c,scale);
}
static void line_heading(u32*d,unsigned s,const char*name) {
    fill(d,s,0,0,SCREEN_W,47,BG);
    centered(d,s,18,name,WHITE,3);
    fill(d,s,20,47,335,1,OUTLINE);
}
static void options(u32*d,unsigned s,const char*heading,const char*const*names,int count,int selected) {
    fill(d,s,57,75,261,229,BLACK);
    border(d,s,57,75,261,229,OUTLINE);
    centered(d,s,89,heading,WHITE,3);
    int spacing=count==4?43:count==3?57:72;
    for(int i=0;i<count;++i){
        int y=123+i*spacing;
        fill(d,s,77,y,221,spacing-8,i==selected?SELECT:PANEL);
        text(d,s,91,y+11,names[i],i==selected?selected_text:DIM,3);
    }
}
/* Compact 3x5 capitals keep bottom hints clear of the rounded glass. */
static void footer(u32 *d,unsigned s,const char *str) {
    static const unsigned char font[26][5]={
        {2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},
        {7,4,6,4,7},{7,4,6,4,4},{3,4,5,5,3},{5,5,7,5,5},
        {7,2,2,2,7},{1,1,1,5,2},{5,5,6,5,5},{4,4,4,4,7},
        {5,7,7,5,5},{5,7,7,7,5},{2,5,5,5,2},{6,5,6,4,4},
        {2,5,5,3,1},{6,5,6,5,5},{3,4,2,1,6},{7,2,2,2,2},
        {5,5,5,5,7},{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},
        {5,5,2,2,2},{7,1,2,4,7}};
    int n=0; while(str[n]) ++n;
    int x=(SCREEN_W-(n*4-1))/2;
    for (;*str;++str,x+=4) {
        if (*str<'A' || *str>'Z') continue;
        for (int y=0;y<5;++y)
            for (int col=0;col<3;++col)
                if (font[*str-'A'][y]&(4>>col)) fill(d,s,x+col,293+y,1,1,DIM);
    }
}
/* Q2 Pod's style conventions: Minimal = accent-framed card;
 * Modern = flat accent selection bar; Classic = light iPod with gloss band.
 * These are native framebuffer approximations, not AWTK theme widgets.
 * Positions intentionally match Arcade's existing wheel/touch hitboxes. */
static u32 blend(u32 a, u32 b, unsigned n, unsigned total) {
    if (n > total) n = total;
    unsigned r = (((a >> 16) & 255u) * (total - n) + ((b >> 16) & 255u) * n) / total;
    unsigned g = (((a >> 8) & 255u) * (total - n) + ((b >> 8) & 255u) * n) / total;
    unsigned bl = ((a & 255u) * (total - n) + (b & 255u) * n) / total;
    return r << 16 | g << 8 | bl;
}
static void menu(u32*d,unsigned stride,int selection,const ArcadeTheme *theme) {
    static const char*const names[]={"PONG","BLOCKS","BREAKOUT"};
    u32 accent=arcade_theme_accent(theme);
    int classic=theme->style==ARCADE_CLASSIC;
    int modern=theme->style==ARCADE_MODERN;
    int bright=classic || theme->light==1 || (theme->style==ARCADE_MINIMAL && theme->accent==7);
    u32 paper=bright?0xf4f4f2:BG;
    u32 ink=bright?0x171b20:WHITE;
    u32 secondary=bright?0x555c64:DIM;
    fill(d,stride,0,0,SCREEN_W,SCREEN_H,paper);

    line_heading(d,stride,"Q2 ARCADE");
    for(int i=0;i<3;++i) {
        int y=MENU_TOP+i*MENU_PITCH;
        int selected=(i==selection);
        if(classic) {
            /* 49px menu hitbox, iPod-style glossy blue/accent highlight.
             * Selection stops inherit Classic's active accent. */
            if(selected) {
                u32 top=theme->accent==9?0x08a6e7:blend(accent,WHITE,34,100);
                u32 bottom=theme->accent==9?0x0065d6:blend(accent,BLACK,20,100);
                for(int n=0;n<MENU_ROW_H;++n)
                    fill(d,stride,27,y+n,321,1,blend(top,bottom,(unsigned)n,MENU_ROW_H-1));
                fill(d,stride,27,y,321,1,blend(top,WHITE,22,100));
                fill(d,stride,27,y+MENU_ROW_H-1,321,1,blend(bottom,BLACK,30,100));
            } else {
                fill(d,stride,27,y+MENU_ROW_H-1,321,1,0xd4d4d4);
            }
            text(d,stride,48,y+11,names[i],selected?WHITE:ink,3);
            if(selected)text(d,stride,311,y+11,">",WHITE,3);
        } else if(modern) {
            fill(d,stride,28,y,319,MENU_ROW_H,selected?accent:(bright?0xfafafa:0x252b35));
            if(!selected)fill(d,stride,28,y+MENU_ROW_H-1,319,1,bright?0xd0d0d0:OUTLINE);
            text(d,stride,48,y+11,names[i],selected?WHITE:ink,3);
            if(selected)text(d,stride,312,y+11,">",WHITE,3);
        } else {
            /* Minimal's accent-framed selection card. */
            u32 tile=bright?0xffffff:0x1e2934;
            u32 edge=selected?accent:(bright?0xc6c8ca:OUTLINE);
            fill(d,stride,30,y+2,315,MENU_ROW_H-4,tile);
            border(d,stride,30,y+2,315,MENU_ROW_H-4,edge);
            if(selected)fill(d,stride,35,y+7,4,MENU_ROW_H-14,accent);
            text(d,stride,53,y+11,names[i],ink,3);
            if(selected)text(d,stride,314,y+11,">",accent,3);
        }
    }
    centered(d,stride,282,"WHEEL SELECT  CENTER OPEN",secondary,1);
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
    footer(d,s,"WHEEL MOVE  RETURN PAUSE");
}
static void blocks(u32*d,unsigned s,const ArcadeApp*a) {
    const Q2Blocks*b=&a->blocks;
    line_heading(d,s,"BLOCKS");
    text(d,s,47,63,"SCORE",DIM,1);
    number(d,s,47,79,b->score,WHITE,3);
    text(d,s,258,63,"LEVEL",DIM,1);
    number(d,s,272,80,b->level,WHITE,3);
    fill(d,s,102,66,112,222,BLACK);
    border(d,s,101,65,114,224,OUTLINE);
    for(int y=0;y<Q2_BOARD_H;++y)
        for(int x=0;x<Q2_BOARD_W;++x){
            int cell=q2_blocks_cell(b,x,y);
            fill(d,s,103+x*11,67+y*11,10,10,
                 cell>0&&cell<8?palette[cell]:PANEL);
        }
    text(d,s,253,130,"NEXT",DIM,3);
    for(int y=0;y<4;++y)
        for(int x=0;x<4;++x){
            int cell=q2_blocks_next_cell(b,x,y);
            if(cell>0&&cell<8)
                fill(d,s,254+x*13,162+y*13,12,12,palette[cell]);
        }
    text(d,s,247,235,"BEST",DIM,1);
    number(d,s,247,248,a->best_blocks,WHITE,1);
    footer(d,s,"WHEEL ROTATE  LEFT RIGHT MOVE");
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
    footer(d,s,"WHEEL MOVE  RETURN PAUSE");
}
void app_render(const ArcadeApp *a, u32 *d, unsigned s) {
    fill(d,s,0,0,SCREEN_W,SCREEN_H,BG);
    if(a->view==VIEW_SELECT){menu(d,s,a->selection,&a->theme);return;}
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
