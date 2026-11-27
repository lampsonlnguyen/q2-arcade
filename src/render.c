/* SPDX-License-Identifier: GPL-3.0-or-later
 * Q2 framebuffer renderer: logical 375x320 landscape, portrait stride.
 * Simple integer rectangles/5x7 font; no image/font assets or system UI.
 */
#include "app.h"
#include "font.h"

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
static int text_width(const char *str,int scale) {
    const unsigned char *adv=scale==1?f12_adv:f18_adv;
    int width=0;
    for(;*str;++str){unsigned c=(unsigned char)*str; if(c>='a'&&c<='z')c-=32;
        width+=adv[c>=32&&c<=90?c-32:0];}
    return width;
}
static void text(u32 *d,unsigned s,int x,int y,const char *str,u32 color,int scale) {
    const unsigned char *adv=scale==1?f12_adv:f18_adv;
    const unsigned short *off=scale==1?f12_off:f18_off;
    const unsigned char *px=scale==1?f12_px:f18_px;
    int height=scale==1?12:17;
    for(;*str;++str) {
        unsigned c=(unsigned char)*str;
        if(c>='a'&&c<='z')c-=32;
        unsigned i=c>=32&&c<=90?c-32:0;
        int width=adv[i];
        for(int row=0;row<height;++row)for(int col=0;col<width;++col) {
            int xx=x+col,yy=y+row;
            unsigned alpha=px[off[i]+row*width+col];
            if(!alpha||!q2_game_ui_safe_point(SCREEN_W,SCREEN_H,xx,yy))continue;
            u32 bg=d[xx*s+SCREEN_H-1-yy];
            unsigned r=(((bg>>16)&255)*(255-alpha)+((color>>16)&255)*alpha)/255;
            unsigned g=(((bg>>8)&255)*(255-alpha)+((color>>8)&255)*alpha)/255;
            unsigned b=((bg&255)*(255-alpha)+(color&255)*alpha)/255;
            fill(d,s,xx,yy,1,1,(r<<16)|(g<<8)|b);
        }
        x+=width;
    }
}
static void centered(u32 *d,unsigned s,int y,const char *str,u32 c,int scale) {
    text(d,s,(SCREEN_W-text_width(str,scale))/2,y,str,c,scale);
}
static void number(u32 *d, unsigned s, int x, int y, unsigned value, u32 c, int scale) {
    char out[12]; int n=0;
    do { out[n++] = (char)('0'+value%10); value/=10; } while(value && n<10);
    for (int i=0; i<n/2; ++i) { char t=out[i]; out[i]=out[n-i-1]; out[n-i-1]=t; }
    out[n]=0; text(d,s,x,y,out,c,scale);
}
static void line_heading(u32*d,unsigned s,const char*name) {
    fill(d,s,0,0,SCREEN_W,47,BG);
    centered(d,s,10,name,WHITE,3);
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
        text(d,s,91,y+8,names[i],i==selected?selected_text:DIM,3);
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
    static const char*const names[]={"SUDOKU","BRICKS","SOLITAIRE","SNAKE","PONG","TETRIS","BREAKOUT",
        "BATTLESHIP","MINESWEEPER","8-BALL POOL"};
    int classic=theme->style==ARCADE_CLASSIC, light=classic||theme->light==1;
    u32 paper=light?0xfafafa:0x101010, ink=light?0x171717:0xeeeeec;
    u32 accent=arcade_theme_accent(theme), rule=light?0xd4d4d4:0x353535;
    u32 header=light?0xe6e6e6:0x222222;
    fill(d,stride,0,0,SCREEN_W,SCREEN_H,paper);
    for(int y=0;y<40;y++)fill(d,stride,0,y,SCREEN_W,1,classic?blend(0xffffff,0xbfc3c8,(unsigned)y,39):header);
    centered(d,stride,12,"ARCADE",ink,3);
    fill(d,stride,0,40,SCREEN_W,1,rule);
    int first=app_menu_first(selection),count=ARCADE_GAME_COUNT-ARCADE_SUDOKU;
    for(int row=0;row<MENU_VISIBLE&&first+row<count;row++){
        int i=first+row,y=MENU_TOP+row*MENU_PITCH,sel=i==selection;
        u32 chosen=accent,fg=0xffffff;
        if(theme->style==ARCADE_MINIMAL&&(theme->accent==0||theme->accent==7)){chosen=ink;fg=paper;}
        if(sel){
            for(int j=0;j<MENU_ROW_H;j++)
                fill(d,stride,17,y+j,341,1,classic?blend(blend(chosen,0xffffff,25,100),blend(chosen,0,15,100),(unsigned)j,MENU_ROW_H-1):chosen);
        }else fill(d,stride,22,y+MENU_ROW_H-1,331,1,rule);
        text(d,stride,29,y+9,names[i],sel?fg:ink,3);
        text(d,stride,334,y+9,">",sel?fg:rule,3);
    }
    int height=MENU_VISIBLE*MENU_PITCH,thumb=height*MENU_VISIBLE/count;
    fill(d,stride,363,MENU_TOP,3,height,rule);
    fill(d,stride,363,MENU_TOP+first*height/count,3,thumb,accent);
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
    line_heading(d,s,"TETRIS");
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
/* Original fixed-point/pixel primitives: no textures, heap or SDL. */
static void disc(u32*d,unsigned s,int cx,int cy,int radius,u32 color){
    for(int y=-radius;y<=radius;y++)
        for(int x=-radius;x<=radius;x++)
            if(x*x+y*y<=radius*radius)fill(d,s,cx+x,cy+y,1,1,color);
}
static void stroke(u32*d,unsigned s,int x0,int y0,int x1,int y1,u32 color,int thick){
    int dx=x1>x0?x1-x0:x0-x1;
    int dy=y1>y0?y1-y0:y0-y1;
    int sx=x0<x1?1:-1,sy=y0<y1?1:-1,err=dx-dy;
    for(;;){
        fill(d,s,x0-thick/2,y0-thick/2,thick,thick,color);
        if(x0==x1&&y0==y1)break;
        int e=2*err;
        if(e>-dy){err-=dy;x0+=sx;}
        if(e<dx){err+=dx;y0+=sy;}
    }
}
static void ocean(u32*d,unsigned s,int x,int y,int w,int h){
    fill(d,s,x,y,w,h,0x075070);
    for(int j=0;j<h;j+=18){
        fill(d,s,x,y+j,w,8,(j/18)%2?0x0e617f:0x0a5777);
        for(int i=8+(j%23);i<w-10;i+=33)
            fill(d,s,x+i,y+j+5,17,1,0x278ca2);
    }
}
static void ship_icon(u32*d,unsigned s,int x,int y,int w,int h,u32 color){
    /* Top-down gunboat: bow, stern, bridge, deck, hatches and wakes. */
    fill(d,s,x+4,y+h/2-3,w-8,6,0x062d43);
    fill(d,s,x+7,y+h/2-5,w-14,10,color);
    fill(d,s,x+12,y+h/2-2,w-24,4,0xc9d5d6);
    fill(d,s,x+w/2-5,y+h/2-4,10,8,0x1a3347);
    fill(d,s,x+w/2-2,y+h/2-7,4,14,0xdfe6d9);
    stroke(d,s,x+4,y+h/2,x,y+h/2-1,0xb0e8e9,1);
    stroke(d,s,x+4,y+h/2,x,y+h/2+1,0xb0e8e9,1);
}
static void battleship(u32*d,unsigned s,const ArcadeApp*a){
    const Q2Mini*g=&a->mini;
    line_heading(d,s,"BATTLESHIP");
    /* Always-visible ship silhouette advertises the naval setting. */
    ocean(d,s,25,49,326,237);
    ship_icon(d,s,23,49,64,16,0x9ebbc4);
    text(d,s,99,51,"HITS",0xddf5fb,1);number(d,s,135,51,g->score,0xffce60,1);
    text(d,s,209,51,"SHOTS",0xddf5fb,1);
    number(d,s,257,51,g->lives-g->moves,0xffce60,1);
    for(int y=0;y<6;y++)for(int x=0;x<6;x++){
        int xx=56+x*44,yy=69+y*36;
        int selected=g->cursor==y*6+x;
        int mark=g->board[y][x];
        int id=(int)g->ship_id[y][x];
        fill(d,s,xx+1,yy+1,42,34,((x+y)&1)?0x116381:0x0d5b79);
        if(id&&(g->ship_hits[id-1]==g->ship_length[id-1]||g->finished)){
            /* Reveal each ship segment with a tiny steel hull. */
            fill(d,s,xx+7,yy+11,30,13,0x223c4d);
            fill(d,s,xx+9,yy+13,26,9,0x98b5bd);
            fill(d,s,xx+19,yy+15,6,5,0xe8f0e8);
        }
        if(mark==1){
            disc(d,s,xx+22,yy+18,11,0xf04c3e);
            disc(d,s,xx+22,yy+18,5,0xffc262);
            stroke(d,s,xx+14,yy+10,xx+30,yy+26,0xffebc4,2);
        }else if(mark==3){
            disc(d,s,xx+22,yy+18,8,0x7ed1e5);
            disc(d,s,xx+22,yy+18,4,0x135575);
        }
        if(selected){
            border(d,s,xx+1,yy+1,42,34,0xffdb61);
            fill(d,s,xx+19,yy+2,6,3,0xffdb61);
        }
    }
    if(g->last_result==3)text(d,s,83,270,"SHIP SUNK",0xffd665,1);
    else if(g->last_result==1)text(d,s,98,270,"DIRECT HIT",0xffd665,1);
    else if(g->last_result==2)text(d,s,112,270,"SPLASH",0xc0e5f5,1);
    footer(d,s,"WHEEL AIM  CENTER FIRE");
}
static void mine_icon(u32*d,unsigned s,int cx,int cy){
    /* Spiked mine with a white glint, readable at one board cell. */
    stroke(d,s,cx-7,cy,cx+7,cy,0x222222,2);
    stroke(d,s,cx,cy-7,cx,cy+7,0x222222,2);
    stroke(d,s,cx-5,cy-5,cx+5,cy+5,0x222222,2);
    stroke(d,s,cx-5,cy+5,cx+5,cy-5,0x222222,2);
    disc(d,s,cx,cy,6,0x222222);
    disc(d,s,cx-2,cy-2,2,0xeeeeec);
}
static void flag_icon(u32*d,unsigned s,int x,int y){
    fill(d,s,x+7,y+5,2,15,0x222222);
    fill(d,s,x+9,y+5,9,5,0xe83d42);
    fill(d,s,x+9,y+10,6,3,0xe83d42);
    fill(d,s,x+5,y+20,7,2,0x222222);
}
static void minesweeper(u32*d,unsigned s,const ArcadeApp*a){
    const Q2Mini*g=&a->mini;
    static const u32 digits[9]={0,0x4a90d9,0x4bc785,0xe86666,
        0x2a4a8a,0x8c2f2f,0x13838d,0,0x949294};
    line_heading(d,s,"MINESWEEPER");
    text(d,s,36,52,"MINES",DIM,1);
    number(d,s,84,52,(unsigned)(g->mine_total-g->flags_placed),WHITE,1);
    text(d,s,240,52,"TIME",DIM,1);
    number(d,s,282,52,g->elapsed_s,WHITE,1);
    fill(d,s,MS_BOARD_X-1,MS_BOARD_Y-1,
         MINI_MS_COLS*MS_CELL+2,MINI_MS_ROWS*MS_CELL+2,OUTLINE);
    for(int row=0;row<MINI_MS_ROWS;row++)for(int col=0;col<MINI_MS_COLS;col++){
        int x=MS_BOARD_X+col*MS_CELL,y=MS_BOARD_Y+row*MS_CELL;
        int mine=g->mines[row][col],show_mine=g->finished&&!g->won&&mine;
        int show_flag=g->flagged[row][col]||(g->finished&&g->won&&mine);
        int hit=g->finished&&!g->won&&mine&&g->cursor==row*MINI_MS_COLS+col;
        if(g->revealed[row][col]&&!show_mine){
            fill(d,s,x,y,MS_CELL,MS_CELL,BG?0xdcdcdc:0x11161c);
            int n=g->adj[row][col];
            if(n){
                char digit[2]={(char)('0'+n),0};
                text(d,s,x+9,y+7,digit,n==7?WHITE:digits[n],1);
            }
        }else{
            fill(d,s,x,y,MS_CELL,MS_CELL,hit?0xe83d42:PANEL);
            if(show_flag)flag_icon(d,s,x,y);
            else if(show_mine)mine_icon(d,s,x+MS_CELL/2,y+MS_CELL/2);
        }
        if(g->cursor==row*MINI_MS_COLS+col)
            border(d,s,x+1,y+1,MS_CELL-2,MS_CELL-2,0xffcf64);
        border(d,s,x,y,MS_CELL,MS_CELL,OUTLINE);
    }
    footer(d,s,"WHEEL MOVE  CENTER REVEAL  PLAY FLAG");
}

static void originals(u32*d,unsigned s,const ArcadeApp*a){
    const OriginalGame*g=&a->original;
    if(g->kind==ORIGINAL_SUDOKU){
        const struct sudoku*b=&g->state.sud;
        line_heading(d,s,"SUDOKU");
        for(int i=0;i<81;i++){
            int x=79+i%9*24,y=58+i/9*24,v=b->cell[i]&15;
            fill(d,s,x,y,24,24,b->cell[i]&CLUE?PANEL:BG);
            if(v)number(d,s,x+8,y+4,(unsigned)v,WHITE,3);
            border(d,s,x,y,24,24,OUTLINE);
            if(i==b->cur)border(d,s,x+1,y+1,22,22,SELECT);
        }
        for(int i=0;i<=9;i+=3){fill(d,s,79+i*24,58,2,216,WHITE);fill(d,s,79,58+i*24,216,2,WHITE);}
        if(g->picking){text(d,s,25,145,"DIGIT",WHITE,1);number(d,s,38,162,(unsigned)g->pick,GOLD,3);}
        footer(d,s,g->picking?"WHEEL DIGIT CENTER SET":"WHEEL MOVE CENTER EDIT");
    }else if(g->kind==ORIGINAL_BRICKS){
        const struct bricks*b=&g->state.bricks;
        line_heading(d,s,"BRICKS");
        number(d,s,45,52,(unsigned)b->score,WHITE,1);number(d,s,306,52,(unsigned)b->lives,WHITE,1);
        for(int ring=0;ring<RINGS;ring++)for(int seg=0;seg<SEGS;seg++)if(b->brick[ring]&(1u<<seg)){
            int rad=R_B+ring*BRICK_T+BRICK_T/2;
            for(int angle=seg*64+3;angle<(seg+1)*64-3;angle+=3)
                disc(d,s,187+rad*original_cos(angle)/65536,174+rad*original_sin(angle)/65536,5,palette[ring+2]);
        }
        for(int angle=b->pa-PAD_HALF;angle<=b->pa+PAD_HALF;angle+=3)
            disc(d,s,187+R_PAD*original_cos(angle)/65536,174+R_PAD*original_sin(angle)/65536,3,CYAN);
        disc(d,s,187+b->x/65536,174+b->y/65536,4,WHITE);
        if(b->state==AIM)centered(d,s,168,"CENTER LAUNCH",WHITE,1);
    }else if(g->kind==ORIGINAL_SOLITAIRE){
        const struct solitaire*b=&g->state.sol;
        static const unsigned char col[PILES]={0,1,3,4,5,6,0,1,2,3,4,5,6};
        static const char *const ranks[]={"A","2","3","4","5","6","7","8","9","10","J","Q","K"};
        static const char *const suits[]={"S","H","D","C"};
        line_heading(d,s,"SOLITAIRE");
        for(int p=0;p<PILES;p++){
            int x=18+49*col[p],y=p<TABLEAU?58:116,n=b->len[p];
            int first=p<TABLEAU&&n?n-1:0,fan=n>9?6:15;
            border(d,s,x,y,45,43,OUTLINE);
            for(int i=first;i<n;i++){
                int card=b->card[p][i],face=card&UP,v=card&~UP,cy=y+(p<TABLEAU?0:i*fan);
                fill(d,s,x,cy,45,43,face?0xfafafa:0x2764a5);
                border(d,s,x,cy,45,43,0x777777);
                if(face){u32 ink=(v/13==1||v/13==2)?0xc82e36:0x161616;text(d,s,x+4,cy+2,ranks[v%13],ink,1);text(d,s,x+27,cy+2,suits[v/13],ink,1);}
            }
            if(p==b->cur|| (g->picking&&p==g->pick))border(d,s,x,y,45,43+(p>=TABLEAU&&n?(n-1)*fan:0),p==b->cur?SELECT:0xffcc66);
        }
        footer(d,s,"WHEEL MOVE CENTER PICK DROP");
    }else{
        const struct snake*b=&g->state.snake;
        line_heading(d,s,"SNAKE");
        number(d,s,45,52,(unsigned)b->score,WHITE,1);
        fill(d,s,77,62,220,220,PANEL);border(d,s,77,62,220,220,OUTLINE);
        for(int i=0;i<b->len;i++){int pos=b->body[(b->tail+i)%SN_CELLS];fill(d,s,78+pos%20*11,63+pos/20*11,9,9,i==b->len-1?WHITE:CYAN);}
        disc(d,s,82+b->food%20*11,67+b->food/20*11,4,0xe86666);
        footer(d,s,"LEFT RIGHT TURN RETURN PAUSE");
    }
}

static void pool_line(u32*d,unsigned s,int x,int y,int tx,int ty,u32 c,int radius){
    int dx=tx-x,dy=ty-y,n=dx<0?-dx:dx,ay=dy<0?-dy:dy;
    if(ay>n)n=ay;
    if(!n)return;
    for(int i=0;i<=n;i++)disc(d,s,x+dx*i/n,y+dy*i/n,radius,c);
}
static void pool_render(u32*d,unsigned s,const ArcadeApp*a){
    const Q2Pool*p=&a->pool;
    static const u32 colors[]={0xf8f8ee,0xf6c332,0x2559cc,0xd02f35,0x753ba2,0xf08025,0x15875b,0x872e28,0x171717};
    line_heading(d,s,"8-BALL POOL");
    text(d,s,12,49,p->turn==0?"PLAYER >":"PLAYER",WHITE,1);
    text(d,s,279,49,p->local?"PLAYER 2":"CPU",p->turn?GOLD:WHITE,1);
    for(int k=0;k<2;k++)for(int i=0;i<7;i++){
        int g=p->group[k],ball=g==2?9+i:1+i;
        int x=k?205+i*10:85+i*10;
        disc(d,s,x,55,4,g&& !p->balls[ball].live?colors[(ball-1)%8+1]:OUTLINE);
    }
    fill(d,s,8,59,359,213,0x512f1b);
    for(int y=0;y<7;y++){
        u32 c=blend(0xb07c43,0x55321c,(unsigned)y,7);
        fill(d,s,12,59+y,351,1,c);fill(d,s,12,265+y,351,1,c);
    }
    fill(d,s,19,66,337,198,0x10482d);
    fill(d,s,POOL_LEFT,POOL_TOP,POOL_RIGHT-POOL_LEFT+1,POOL_BOTTOM-POOL_TOP+1,0x206a3d);
    for(int y=POOL_TOP+2;y<POOL_BOTTOM-1;y++){
        int dy=y-160;if(dy<0)dy=-dy;
        fill(d,s,POOL_LEFT+2,y,POOL_RIGHT-POOL_LEFT-3,1,blend(0x2c8c4e,0x185c35,(unsigned)dy,95));
    }
    for(int i=0;i<3;i++)for(int k=0;k<2;k++){
        int x=i==0?POOL_LEFT:i==1?187:POOL_RIGHT,y=k?POOL_BOTTOM:POOL_TOP;
        disc(d,s,x,y,11,0x151e17);disc(d,s,x,y,9,0x030604);
    }
    for(int x=70;x<340;x+=38){disc(d,s,x,62,1,0xe8d8ba);disc(d,s,x,268,1,0xe8d8ba);}
    fill(d,s,100,78,1,164,0x438b56);
    const PoolBall *cue=&p->balls[0];
    if(cue->live&&!p->moving&&!p->ended&&!p->ball_in_hand){
        int x=cue->x/POOL_SCALE,y=cue->y/POOL_SCALE,dx=original_cos(p->angle)/256,dy=original_sin(p->angle)/256;
        int lastx=x,lasty=y;
        for(int i=8;i<320;i+=3){
            int xx=x+dx*i/256,yy=y+dy*i/256,hit=0;
            if(xx<POOL_LEFT+6||xx>POOL_RIGHT-6||yy<POOL_TOP+6||yy>POOL_BOTTOM-6)break;
            for(int j=1;j<16;j++)if(p->balls[j].live){
                int bx=xx-p->balls[j].x/POOL_SCALE,by=yy-p->balls[j].y/POOL_SCALE;
                if(bx*bx+by*by<144){hit=1;break;}
            }
            lastx=xx;lasty=yy;
            if(hit)break;
            if(i%2==0)disc(d,s,xx,yy,1,0x9ce3b2);
        }
        for(int i=0;i<16;i++)disc(d,s,lastx+original_cos(i*64)*6/65536,lasty+original_sin(i*64)*6/65536,1,0xc6efce);
        int off=12+(p->charging?p->power/5:0);
        pool_line(d,s,x-dx*(off+65)/256,y-dy*(off+65)/256,x-dx*off/256,y-dy*off/256,0x543623,2);
        pool_line(d,s,x-dx*(off+65)/256,y-dy*(off+65)/256,x-dx*off/256,y-dy*off/256,0xe6bd76,1);
    }
    for(int i=0;i<16;i++)if(p->balls[i].live){
        int x=p->balls[i].x/POOL_SCALE,y=p->balls[i].y/POOL_SCALE;
        u32 c=colors[i>8?i-8:i];
        disc(d,s,x+1,y+2,7,0x16412a);disc(d,s,x,y,6,i>8?0xf2f1e9:c);
        if(i>8)for(int yy=-3;yy<=3;yy++){int w=yy==3||yy==-3?4:5;fill(d,s,x-w,y+yy,w*2+1,1,c);}
        if(i){disc(d,s,x,y,3,0xf9f9f5);/* tiny binary-coded number glyph */
            static const unsigned short digits[]={0x7b6f,0x2492,0x73e7,0x73cf,0x5bc9,0x79cf,0x79ef,0x7249,0x7bef,0x7bcf};
            int n=i%10;for(int yy=0;yy<5;yy++)for(int xx=0;xx<3;xx++)if(digits[n]&(1u<<(14-yy*3-xx)))fill(d,s,x-1+xx,y-2+yy,1,1,0x242424);
        }else disc(d,s,x-2,y-2,2,0xffffff);
    }
    text(d,s,15,279,"POWER",WHITE,1);
    fill(d,s,60,281,155,9,OUTLINE);fill(d,s,60,281,p->power*155/100,9,0xe8bc42);
    text(d,s,232,279,p->ball_in_hand?"PLACE CUE":p->moving?"ROLLING":p->turn&&!p->local?"CPU AIMING":"AIM / SHOOT",WHITE,1);
    footer(d,s,p->ball_in_hand?"TOUCH TABLE TO PLACE CUE":"WHEEL AIM  HOLD CENTER SHOOT  PLAY POWER");
}

void app_render(const ArcadeApp *a, u32 *d, unsigned s) {
    fill(d,s,0,0,SCREEN_W,SCREEN_H,BG);
    if(a->view==VIEW_SELECT){menu(d,s,a->selection,&a->theme);return;}
    if(a->game>=ARCADE_SUDOKU&&a->game<=ARCADE_SNAKE)originals(d,s,a);
    else if(a->game==ARCADE_PONG)pong(d,s,a);
    else if(a->game==ARCADE_BLOCKS)blocks(d,s,a);
    else if(a->game==ARCADE_BREAKOUT)breakout(d,s,a);
    else if(a->game==ARCADE_BATTLE)battleship(d,s,a);
    else if(a->game==ARCADE_POOL)pool_render(d,s,a);
    else minesweeper(d,s,a);
    if(a->view==VIEW_PAUSE) {
        static const char* names[]={"RESUME","RESTART","GAMES","EXIT"};
        options(d,s,"PAUSED",names,4,a->option);
    } else if(a->view==VIEW_OVER) {
        static const char* names[]={"RESTART","GAMES"};
        const char *heading="GAME OVER";
        if(a->game>=ARCADE_SUDOKU&&a->game<=ARCADE_SNAKE&&a->original.won)heading="YOU WIN";
        if(a->game==ARCADE_POOL)heading=a->pool.winner==0?"YOU WIN":"CPU WINS";
        if(a->game>=ARCADE_BATTLE&&a->game<=ARCADE_MINES) {
            if(a->mini.outcome==1)heading=a->game==ARCADE_BATTLE?"FLEET SUNK":"YOU WIN";
            else if(a->mini.outcome==2)heading="CPU WINS";
            else if(a->game==ARCADE_BATTLE)heading="OUT OF SHOTS";
            else if(a->game==ARCADE_MINES)heading="MINE HIT";
        }
        options(d,s,heading,names,2,a->option);
    }
}
