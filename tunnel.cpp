#include "rfifo.h"
#include <windows.h>

 

/* just one direction now */
typedef struct tunnel {
	rfifo_t buf;
	HANDLE ev; /* when have data, set it after filling buf; when buf is full, wait for it? */
} tunnel;
