#ifndef _MYPORTLISTENER_H
#define _MYPORTLISTENER_H

#include "mytunnel.h"

cliptund::ConnectionFactory *tcp_CreateConnectionFactory(const char host[40+1], short port);
int tcp_create_listener(short port, cliptund::ConnectionFactory *connfact);

#endif
