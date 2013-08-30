#ifndef _MYCLIPSERVER_H
#define _MYCLIPSERVER_H

#include "mytunnel.h"

int clipsrv_init();
int clipsrv_create_listener(const char clipname[40+1], const char host[40+1], short port);
void clipsrv_connect(cliptund::Tunnel *tun, const char *clipname);

#endif
