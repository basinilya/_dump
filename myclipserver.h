#ifndef _CLIPTUN_CLIPSERVER_H
#define _CLIPTUN_CLIPSERVER_H

#include "mytunnel.h"

void clipsrv_init();
void clipsrv_create_listener(cliptund::ConnectionFactory *connfact, const char clipname[40+1]);
cliptund::ConnectionFactory *clipsrv_CreateConnectionFactory(const char clipname[40+1]);

#endif
