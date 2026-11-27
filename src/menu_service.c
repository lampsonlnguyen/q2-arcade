/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "browse.h"
extern long sc(long,long,long,long,long,long,long);
enum {READ=4003,CLOSE=4006,UNLINK=4010,ACCEPT=4168,BIND=4169,
      LISTEN=4174,SEND=4178,SETSOCKOPT=4181,SOCKET=4183};
#define SOCK "/tmp/q2-arcade.browse.sock"
static void close_fd(int fd){sc(CLOSE,fd,0,0,0,0,0);}
static void reply(int fd){
    char request[8193],out[1024];unsigned used=0;
    long timeout[2]={2,0};
    sc(SETSOCKOPT,fd,0xffff,0x1006,(long)timeout,sizeof timeout,0);
    sc(SETSOCKOPT,fd,0xffff,0x1005,(long)timeout,sizeof timeout,0);
    while(used<sizeof request-1){
        long n=sc(READ,fd,(long)(request+used),sizeof request-1-used,0,0,0);
        if(n==-4)continue;
        if(n<0)return;
        if(!n)break;
        used+=(unsigned)n;
    }
    if(used==sizeof request-1)return;
    request[used]=0;
    size_t len=arcade_menu_reply(request,out,sizeof out),sent=0;
    while(sent<len){
        long n=sc(SEND,fd,(long)(out+sent),len-sent,0x4000,0,0);
        if(n==-4)continue;
        if(n<=0)break;
        sent+=(size_t)n;
    }
}
int arcade_menu_serve(void){
    struct {unsigned short family;char path[108];} addr={1,SOCK};
    int fd=(int)sc(SOCKET,1,2,0,0,0,0);
    if(fd<0)return 1;
    sc(UNLINK,(long)SOCK,0,0,0,0,0);
    if(sc(BIND,fd,(long)&addr,sizeof addr,0,0,0)||sc(LISTEN,fd,4,0,0,0,0)){
        close_fd(fd);sc(UNLINK,(long)SOCK,0,0,0,0,0);return 1;
    }
    for(;;){
        int client=(int)sc(ACCEPT,fd,0,0,0,0,0);
        if(client==-4)continue;
        if(client<0)break;
        reply(client);close_fd(client);
    }
    close_fd(fd);sc(UNLINK,(long)SOCK,0,0,0,0,0);return 1;
}
