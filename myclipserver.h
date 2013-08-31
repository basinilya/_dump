#ifndef _MYCLIPSERVER_H
#define _MYCLIPSERVER_H

#include "mytunnel.h"

int clipsrv_init();
int clipsrv_create_listener(cliptund::ConnectionFactory *connfact, const char clipname[40+1]);
cliptund::ConnectionFactory *clipsrv_CreateConnectionFactory(const char clipname[40+1]);

#endif
