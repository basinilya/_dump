#ifndef _CLIPTUN_PORTLISTENER_H
#define _CLIPTUN_PORTLISTENER_H

#include "mytunnel.h"

cliptund::ConnectionFactory *tcp_CreateConnectionFactory(const char host[40+1], short port);
int tcp_create_listener(cliptund::ConnectionFactory *connfact, short port);

#endif
