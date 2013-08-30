#ifndef _RFIFO_H
#define _RFIFO_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RFIFO_BUFSZ 2048

typedef struct rfifo_t {
    size_t ofs_beg;
    size_t ofs_end;
	size_t ofs_mid;
    char data[RFIFO_BUFSZ];
} rfifo_t;

size_t rfifo_availread(rfifo_t *rfifo);
char *rfifo_pdata(rfifo_t *rfifo);
size_t rfifo_availwrite(rfifo_t *rfifo);
char *rfifo_pfree(rfifo_t *rfifo);
void rfifo_init(rfifo_t *rfifo);
void rfifo_markwrite(rfifo_t *rfifo, size_t count);

void rfifo_markread(rfifo_t *rfifo, size_t count);
void rfifo_confirmread(rfifo_t *rfifo, size_t count);

#ifdef __cplusplus
}
#endif

#endif
