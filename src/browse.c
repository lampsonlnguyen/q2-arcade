/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "app.h"
#include "browse.h"
static const struct {const char *uri,*title;int game;} games[]={
    {"sudoku","Sudoku",ARCADE_SUDOKU},{"bricks","Bricks",ARCADE_BRICKS},
    {"solitaire","Solitaire",ARCADE_SOLITAIRE},{"snake","Snake",ARCADE_SNAKE},
    {"pong","Pong",ARCADE_PONG},{"tetris","Tetris",ARCADE_BLOCKS},
    {"breakout","Breakout",ARCADE_BREAKOUT},{"battleship","Battleship",ARCADE_BATTLE},
    {"minesweeper","Minesweeper",ARCADE_MINES},{"pool","8-Ball Pool",ARCADE_POOL}
};
static int eq(const char *a,const char *b){
    if(!a||!b)return 0;
    while(*a&&*a==*b){++a;++b;}
    return *a==*b;
}
int arcade_uri(const char *uri){
    for(unsigned i=0;i<sizeof games/sizeof games[0];++i)
        if(eq(uri,games[i].uri))return games[i].game;
    return ARCADE_MENU;
}
const char *arcade_env(char **env,const char *key){
    for(;env&&*env;++env){
        const char *p=*env,*k=key;
        while(*k&&*p==*k){++p;++k;}
        if(!*k&&*p=='=')return p+1;
    }
    return 0;
}
static int append(char **at,size_t *left,const char *s){
    while(*s){if(*left<=1)return 0;*(*at)++=*s++;--*left;}
    **at=0;return 1;
}
size_t arcade_menu_reply(const char *req,char *out,size_t capacity){
    const char *magic="Q2SPOT/1\n";
    if(!req||!out||!capacity)return 0;
    while(*magic)if(*req++!=*magic++)return 0;
    char id[24],op[24];unsigned n=0;
    while(*req>='0'&&*req<='9'&&n<23)id[n++]=*req++;
    if(!n||*req++!='\t')return 0;
    id[n]=0;n=0;
    while(*req&&*req!='\t'&&*req!='\r'&&*req!='\n'&&n<23)op[n++]=*req++;
    if(n==23&&*req&&*req!='\t'&&*req!='\r'&&*req!='\n')return 0;
    op[n]=0;
    char *at=out;size_t left=capacity;int menu=eq(op,"menu");
    if(!append(&at,&left,"Q2SPOT/1\n")||!append(&at,&left,id)||
       !append(&at,&left,menu?"\tok\t\t\n":"\tunsupported\t\t\n"))return 0;
    if(menu)for(unsigned i=0;i<sizeof games/sizeof games[0];++i){
        if(!append(&at,&left,"screen\t")||!append(&at,&left,games[i].uri)||
           !append(&at,&left,"\t")||!append(&at,&left,games[i].title)||
           !append(&at,&left,"\t\t\t\n"))return 0;
    }
    return (size_t)(at-out);
}
