#ifndef _MYCLIPSERVER_H
#define _MYCLIPSERVER_H

#include <rpc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum cnnstate {
	STATE_SYN
} cnnstate;

typedef struct clip_connection {
	cnnstate state;
	union {
		char clipname[40+1];
	} a;
} clip_connection;

typedef struct clipaddr {
	UUID addr;
	int nchannel;
} clipaddr;

int clipsrv_reg_cnn(clip_connection *conn);
int clipsrv_init();
int clipsrv_havenewdata();
int clipsrv_connect(const char *clipname, HANDLE ev, clipaddr *remote);

#ifdef __cplusplus
}
#endif

#endif
