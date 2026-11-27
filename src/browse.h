/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef ARCADE_BROWSE_H
#define ARCADE_BROWSE_H
#include <stddef.h>
int arcade_uri(const char *uri);
const char *arcade_env(char **env,const char *key);
size_t arcade_menu_reply(const char *request,char *out,size_t capacity);
int arcade_menu_serve(void);
#endif
