#ifndef _RFIFO_H
#define _RFIFO_H

#include <stdlib.h>
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RFIFO_BUFSZ 2048

typedef LONG rfifo_long;

typedef struct rfifo_t {
	volatile rfifo_long ofs_beg;
	volatile rfifo_long ofs_end;
	volatile rfifo_long ofs_mid;
	char data[RFIFO_BUFSZ];
} rfifo_t;

rfifo_long rfifo_availread(rfifo_t *rfifo);
char *rfifo_pdata(rfifo_t *rfifo);
rfifo_long rfifo_availwrite(rfifo_t *rfifo);
char *rfifo_pfree(rfifo_t *rfifo);
void rfifo_init(rfifo_t *rfifo);
void rfifo_markwrite123(rfifo_t *rfifo, rfifo_long count);

void rfifo_markread(rfifo_t *rfifo, rfifo_long count);
void rfifo_confirmread(rfifo_t *rfifo, rfifo_long count);

#ifdef __cplusplus
}
#endif

#endif
