#define INLINE __inline

#include <windows.h>
#include <stdlib.h>

#define RFIFO_BUFSZ 8

typedef struct rfifo_t {
    size_t ofs_beg;
    size_t ofs_end;
    char data[RFIFO_BUFSZ];
} rfifo_t;

enum { _assert_RFIFO_BUFSZ__pow2 = 1/(int)!((RFIFO_BUFSZ)&((RFIFO_BUFSZ)-1)) };

size_t rfifo_availread(rfifo_t *rfifo)
{
	size_t ofs_end = rfifo->ofs_end;
	size_t ofs_beg = rfifo->ofs_beg;

	if (ofs_end == ofs_beg) return 0;

	size_t rem_beg = ofs_beg % RFIFO_BUFSZ;
	size_t rem_end = ofs_end % RFIFO_BUFSZ;

	size_t x;
	x = ofs_end - ofs_beg;
	return (ofs_end - ofs_beg) % RFIFO_BUFSZ;

	if (rem_end > rem_beg) return rem_end - rem_beg;
	if (rem_end == rem_beg) return RFIFO_BUFSZ - rem_beg;
	if (rem_end < rem_beg) return RFIFO_BUFSZ - rem_beg;
}

char *rfifo_pdata(rfifo_t *rfifo)
{
	return rfifo->data + (rfifo->ofs_beg % RFIFO_BUFSZ);
}

size_t availread;
size_t pdata;
size_t availwrite;
size_t pfree;

size_t rfifo_availwrite(rfifo_t *rfifo)
{
	size_t ofs_end = rfifo->ofs_end;
	size_t ofs_beg = rfifo->ofs_beg;

	size_t rem_beg = ofs_beg % RFIFO_BUFSZ;
	size_t rem_end = ofs_end % RFIFO_BUFSZ;

	if (ofs_end > ofs_beg) {
		if (rem_end <= rem_beg) {
			return rem_beg - rem_end;
		}
	}
	return RFIFO_BUFSZ - rem_end;

	if (ofs_end == ofs_beg) {
		/* 0,0 - 8
		   8,8 - 8
		   1,1 - 7
		   */
		return RFIFO_BUFSZ - rem_end;
	}
	if (ofs_end > ofs_beg) {
		if (rem_end > rem_beg) {
			return RFIFO_BUFSZ - rem_end;
		}
		if (rem_end <= rem_beg) {
			return (ofs_beg - ofs_end) % RFIFO_BUFSZ;
		}
	}
	return -1;
}

char *rfifo_pfree(rfifo_t *rfifo)
{
	return rfifo->data + (rfifo->ofs_end % RFIFO_BUFSZ);
}

static struct {
	size_t availread;
	size_t pdata;
	size_t availwrite;
	size_t pfree;
	rfifo_t rfifo;
} x[] = {
	{ 0,1,7,1, { 1,1 } },
	{ 0,0,8,0, { 0,0 } },

	{ 1,1,6,2, { 1,2 } },
	{ 2,0,6,2, { 0,2 } },
	{ 1,0,7,1, { 0,1 } },
	{ 7,1,1,0, { 1,8 } },
	{ 8,0,0,0, { 0,8 } },
	{ 7,1,0,1, { 1,9 } },
	{ 6,2,1,1, { 2,9 } },
	{ 1,7,7,0, { 7,8 } },
	{ -1 }

};

#include <assert.h>

int main()
{
	int i, j;
	for (j = 0; j < 2; j++) {
		for (i = 0; x[i].availread != -1; i++) {
			x[i].rfifo.ofs_beg += RFIFO_BUFSZ;
			x[i].rfifo.ofs_end += RFIFO_BUFSZ;

			 availread = rfifo_availread(&x[i].rfifo);
			 pdata = rfifo_pdata(&x[i].rfifo) - x[i].rfifo.data;
			 availwrite = rfifo_availwrite(&x[i].rfifo);
			 pfree = rfifo_pfree(&x[i].rfifo) - x[i].rfifo.data;

			//assert(availread == x[i].availread);
			assert(pdata == x[i].pdata);
			assert(availwrite == x[i].availwrite);
			assert(pfree == x[i].pfree);

		}
	}
	return 0;
}
/*
typedef struct rfifo_t {
    size_t size;
    size_t oData;
    size_t count;
    char data[2048];
} rfifo_t;

#define RFIFO_OFREE(_prfifo) ( ((_prfifo)->oData + (_prfifo)->count) % (_prfifo)->size )

#define RFIFO_PFREE(_prfifo) ((_prfifo)->data+RFIFO_OFREE(_prfifo))
#define RFIFO_PDATA(_prfifo) ((_prfifo)->data+(_prfifo)->oData)

#define RFIFO_AVAILREAD(_prfifo) MIN((_prfifo)->count,(_prfifo)->size - (_prfifo)->oData)
#define RFIFO_AVAILWRITE(_prfifo) ((_prfifo)->oData + (_prfifo)->count >= (_prfifo)->size ? (_prfifo)->size - (_prfifo)->count : (_prfifo)->size - ((_prfifo)->oData + (_prfifo)->count))

#define RFIFO_ISFULL(_prfifo) (RFIFO_AVAILWRITE(_prfifo)==0)
#define RFIFO_ISEMPTY(_prfifo) (RFIFO_AVAILREAD(_prfifo)==0)

static INLINE void rfifo_markwrite(rfifo_t *rfifo, size_t count)
{
    rfifo->count += count;
}

static INLINE void rfifo_markread(rfifo_t *rfifo, size_t count)
{
    rfifo->count -= count;
    if ((rfifo->oData += count) == rfifo->size)
    {
        rfifo->oData = 0;
    }
}

static void rfifo_init(rfifo_t *rfifo, size_t bufSize)
{
    rfifo->size = bufSize;
    rfifo->oData = 0;
    rfifo->count = 0;
}

*/
