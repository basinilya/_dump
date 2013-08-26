#ifndef _MYCLIPSERVER_H
#define _MYCLIPSERVER_H

#include <rpc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct clipaddr {
	UUID addr;
	int nchannel;
} clipaddr;

int clipsrv_init();
int clipsrv_connect(const char *clipname, HANDLE ev, clipaddr *remote);

#ifdef __cplusplus
}
#endif

#endif
