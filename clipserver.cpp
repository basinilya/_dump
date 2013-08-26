#include "myclipserver.h"

#include <windows.h>
#include <rpc.h>

static UUID localclipuuid;
static int nchannel;

int clipsrv_init()
{
	UuidCreate(&localclipuuid);
	return 0;
}

int clipsrv_connect(const char *clipname, HANDLE ev, clipaddr *remote)
{
	return 0;
}
