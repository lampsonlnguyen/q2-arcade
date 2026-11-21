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
    static const char*const names[]={"PONG","BLOCKS","BREAKOUT",
        "TIC TAC TOE","CONNECT FOUR","BATTLESHIP","ROAD DODGE","MINESWEEPER"};
    u32 accent=arcade_theme_accent(theme);
    int classic=theme->style==ARCADE_CLASSIC;
    int modern=theme->style==ARCADE_MODERN;
    int bright=classic || theme->light==1 || (theme->style==ARCADE_MINIMAL && theme->accent==7);
    u32 paper=bright?0xf4f4f2:BG;
    u32 ink=bright?0x171b20:WHITE;
    u32 secondary=bright?0x555c64:DIM;
    fill(d,stride,0,0,SCREEN_W,SCREEN_H,paper);

    line_heading(d,stride,"Q2 ARCADE");
    int first=selection>=4?selection-3:0;
    for(int row=0;row<4;++row) {
        int i=first+row;
        if(i>=ARCADE_GAME_COUNT-ARCADE_PONG)break;
        int y=MENU_TOP+row*MENU_PITCH;
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
            text(d,stride,48,y+8,names[i],selected?WHITE:ink,3);
            if(selected)text(d,stride,311,y+8,">",WHITE,3);
        } else if(modern) {
            fill(d,stride,28,y,319,MENU_ROW_H,selected?accent:(bright?0xfafafa:0x252b35));
            if(!selected)fill(d,stride,28,y+MENU_ROW_H-1,319,1,bright?0xd0d0d0:OUTLINE);
            text(d,stride,48,y+8,names[i],selected?WHITE:ink,3);
            if(selected)text(d,stride,312,y+8,">",WHITE,3);
        } else {
            /* Minimal's accent-framed selection card. */
            u32 tile=bright?0xffffff:0x1e2934;
            u32 edge=selected?accent:(bright?0xc6c8ca:OUTLINE);
            fill(d,stride,30,y+2,315,MENU_ROW_H-4,tile);
            border(d,stride,30,y+2,315,MENU_ROW_H-4,edge);
            if(selected)fill(d,stride,35,y+7,4,MENU_ROW_H-14,accent);
            text(d,stride,53,y+8,names[i],ink,3);
            if(selected)text(d,stride,314,y+8,">",accent,3);
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
/* Packed transparent sprites, tinted and alpha blended without a PNG decoder. */
#include "ttt_sprites.h"
static void ttt_piece(u32*d,unsigned s,int x,int y,int size,int mark){
    u32 color=mark==1?0x72dbf7:0xffcc66;
    for(int yy=0;yy<size;yy++)for(int xx=0;xx<size;xx++){
        unsigned alpha=ttt_alpha[mark-1][(yy*64/size)*64+xx*64/size];
        if(alpha)fill(d,s,x+xx,y+yy,1,1,blend(0x18283a,color,alpha,255));
    }
}
static void tic_setup(u32*d,unsigned s,const ArcadeApp*a){
    static const char*const boards[]={"BOARD  3 X 3","BOARD  6 X 6","BOARD  9 X 9"};
    static const char*const rules[]={"3 IN A ROW TO WIN","4 IN A ROW TO WIN","5 IN A ROW TO WIN"};
    const char*rows[]={a->ttt_pvp?"PLAYER VS PLAYER":"PLAYER VS AI",boards[a->ttt_size_option],"START GAME"};
    line_heading(d,s,"TIC TAC TOE");
    centered(d,s,65,"GAME SETUP",DIM,2);
    for(int i=0;i<3;i++){
        fill(d,s,47,100+i*48,281,40,i==a->option?SELECT:PANEL);
        centered(d,s,112+i*48,rows[i],i==a->option?selected_text:WHITE,2);
    }
    centered(d,s,260,rules[a->ttt_size_option],DIM,2);
    footer(d,s,"WHEEL SELECT  CENTER CHANGE");
}
static void tic_tac(u32*d,unsigned s,const ArcadeApp*a){
    const Q2Mini*g=&a->mini;
    int n=g->ttt_size,cell=TTT_BOARD_PIXELS/n,left=(SCREEN_W-cell*n)/2;
    line_heading(d,s,"TIC TAC TOE");
    const char*status=g->ttt_pvp?(g->ttt_turn==1?"PLAYER 1 - X TURN":"PLAYER 2 - O TURN"):a->ttt_ai_remaining?"AI THINKING...":"YOU ARE X - AI IS O";
    centered(d,s,54,status,DIM,2);
    fill(d,s,left,TTT_BOARD_Y,cell*n,cell*n,0x18283a);
    for(int j=1;j<n;j++){
        fill(d,s,left+j*cell,TTT_BOARD_Y,1,cell*n,0x54788e);
        fill(d,s,left,TTT_BOARD_Y+j*cell,cell*n,1,0x54788e);
    }
    for(int i=0;i<n*n;i++){
        int col=i%n,row=i/n,x=left+col*cell,y=TTT_BOARD_Y+row*cell;
        if(i==g->cursor)border(d,s,x+2,y+2,cell-3,cell-3,0xffcf64);
        int mark=g->board[row][col],size=cell-8;
        if(mark)ttt_piece(d,s,x+(cell-size)/2,y+(cell-size)/2,size,mark);
    }
    footer(d,s,"WHEEL MOVE  CENTER PLACE");
}
static void connect_four(u32*d,unsigned s,const ArcadeApp*a){
    const Q2Mini*g=&a->mini;
    line_heading(d,s,"CONNECT FOUR");
    text(d,s,56,52,"RED YOU",0xf2a2a2,1);
    text(d,s,242,52,"YELLOW CPU",0xf5d878,1);
    /* Bright blue game frame with recessed round disc slots. */
    fill(d,s,33,73,309,212,0x163b82);
    fill(d,s,39,79,297,200,0x2466c7);
    for(int y=0;y<6;y++)for(int x=0;x<7;x++){
        int cx=61+x*42,cy=97+y*33;
        disc(d,s,cx+1,cy+2,15,0x103567);
        u32 color=g->board[y][x]==1?0xe83d42:
                  g->board[y][x]==2?0xffd847:0x0b2034;
        disc(d,s,cx,cy,14,color);
        if(g->board[y][x]){
            disc(d,s,cx-4,cy-5,4,g->board[y][x]==1?0xff8b82:0xffee9a);
        }else disc(d,s,cx-4,cy-5,3,0x24476d);
        if(x==g->cursor&&y==0)
            fill(d,s,cx-18,76,36,2,0xffec75);
    }
    disc(d,s,61+g->cursor*42,67,6,0xffd847);
    footer(d,s,"WHEEL COLUMN  CENTER DROP");
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
static void car(u32*d,unsigned s,int cx,int y,u32 body,int enemy){
    /* Readable top-down automobile: tires, hood, windshield, cabin,
       headlights and rear lights. */
    fill(d,s,cx-19,y+9,5,11,0x111923);
    fill(d,s,cx+14,y+9,5,11,0x111923);
    fill(d,s,cx-19,y+29,5,11,0x111923);
    fill(d,s,cx+14,y+29,5,11,0x111923);
    fill(d,s,cx-15,y+3,30,44,0x172332);
    fill(d,s,cx-13,y+4,26,42,body);
    fill(d,s,cx-10,y+14,20,13,enemy?0x7fbbd0:0xa6e0f4);
    fill(d,s,cx-10,y+29,20,5,0x253b55);
    fill(d,s,cx-11,y+5,7,4,enemy?0xfc8c74:0xffed91);
    fill(d,s,cx+4,y+5,7,4,enemy?0xfc8c74:0xffed91);
    fill(d,s,cx-11,y+39,7,4,enemy?0xfff3cc:0xf54b47);
    fill(d,s,cx+4,y+39,7,4,enemy?0xfff3cc:0xf54b47);
    fill(d,s,cx-2,y+14,4,13,0xd6eef5);
}
static void road_dodge(u32*d,unsigned s,const ArcadeApp*a){
    const Q2Mini*g=&a->mini;
    line_heading(d,s,"ROAD DODGE");
    text(d,s,36,52,"SCORE",DIM,1);number(d,s,82,52,g->score,WHITE,1);
    text(d,s,239,52,"LIVES",DIM,1);number(d,s,286,52,g->lives,WHITE,1);
    fill(d,s,28,68,319,220,0x357d48);
    /* Edge grass, asphalt, curb and dashed moving lane markers. */
    fill(d,s,77,69,221,218,0x65757b);
    fill(d,s,81,69,213,218,0x343b45);
    fill(d,s,83,69,3,218,0xffd568);
    fill(d,s,289,69,3,218,0xffd568);
    for(int lane=1;lane<3;lane++)
        for(int y=79+(g->obstacle_y*4)%22;y<281;y+=28)
            fill(d,s,80+lane*71,y,3,14,0xe9e9db);
    if(g->obstacle_y>=0){
        int oy=72+g->obstacle_y*23;
        if(oy<235)car(d,s,116+g->obstacle*71,oy,0xf0a632,1);
    }
    car(d,s,116+g->lane*71,232,0x23b9d5,0);
    footer(d,s,"WHEEL OR LEFT RIGHT STEER");
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

void app_render(const ArcadeApp *a, u32 *d, unsigned s) {
    fill(d,s,0,0,SCREEN_W,SCREEN_H,BG);
    if(a->view==VIEW_SELECT){menu(d,s,a->selection,&a->theme);return;}
    if(a->view==VIEW_TTT_SETUP){tic_setup(d,s,a);return;}
    if(a->game==ARCADE_PONG)pong(d,s,a);
    else if(a->game==ARCADE_BLOCKS)blocks(d,s,a);
    else if(a->game==ARCADE_BREAKOUT)breakout(d,s,a);
    else if(a->game==ARCADE_TTT)tic_tac(d,s,a);
    else if(a->game==ARCADE_CONNECT)connect_four(d,s,a);
    else if(a->game==ARCADE_BATTLE)battleship(d,s,a);
    else if(a->game==ARCADE_ROAD)road_dodge(d,s,a);
    else minesweeper(d,s,a);
    if(a->view==VIEW_PAUSE) {
        static const char* names[]={"RESUME","RESTART","GAMES","EXIT"};
        options(d,s,"PAUSED",names,4,a->option);
    } else if(a->view==VIEW_OVER) {
        static const char* names[]={"RESTART","GAMES"};
        const char *heading="GAME OVER";
        if(a->game>=ARCADE_TTT) {
            if(a->mini.outcome==1)heading=a->game==ARCADE_BATTLE?"FLEET SUNK":"YOU WIN";
            else if(a->mini.outcome==2)heading="CPU WINS";
            else if(a->game==ARCADE_BATTLE)heading="OUT OF SHOTS";
            else if(a->game==ARCADE_MINES)heading="MINE HIT";
            else if(a->game==ARCADE_TTT||a->game==ARCADE_CONNECT)heading="DRAW";
        }
        if(a->game==ARCADE_TTT&&a->mini.ttt_pvp){
            if(a->mini.outcome==1)heading="X WINS";
            else if(a->mini.outcome==2)heading="O WINS";
        }
        options(d,s,heading,names,2,a->option);
    }
}
