#include "rfifo.h"

size_t rfifo_availread(rfifo_t *rfifo)
{
	size_t ofs_end = rfifo->ofs_end;
	size_t ofs_beg = rfifo->ofs_beg;

	if (ofs_end != ofs_beg) {
		size_t rem_end = ofs_end % RFIFO_BUFSZ;
		size_t rem_beg = ofs_beg % RFIFO_BUFSZ;

		return (rem_end <= rem_beg ? RFIFO_BUFSZ : rem_end) - rem_beg;
	}
	return 0;
}

char *rfifo_pdata(rfifo_t *rfifo)
{
	return rfifo->data + (rfifo->ofs_beg % RFIFO_BUFSZ);
}

size_t rfifo_availwrite(rfifo_t *rfifo)
{
	size_t ofs_end = rfifo->ofs_end;
	size_t ofs_beg = rfifo->ofs_beg;

	size_t rem_end = ofs_end % RFIFO_BUFSZ;

	if (ofs_end != ofs_beg) {
		size_t rem_beg = ofs_beg % RFIFO_BUFSZ;
		if (rem_end <= rem_beg) {
			return rem_beg - rem_end;
		}
	}
	return RFIFO_BUFSZ - rem_end;
}

char *rfifo_pfree(rfifo_t *rfifo)
{
	return rfifo->data + (rfifo->ofs_end % RFIFO_BUFSZ);
}

void rfifo_init(rfifo_t *rfifo)
{
	rfifo->ofs_beg = 0;
	rfifo->ofs_end = 0;
}

void rfifo_markwrite(rfifo_t *rfifo, size_t count)
{
    rfifo->ofs_end += count;
}

void rfifo_markread(rfifo_t *rfifo, size_t count)
{
	rfifo->ofs_beg += count;
}

#if 0

size_t availread;
size_t pdata;
size_t availwrite;
size_t pfree;


/* RFIFO_BUFSZ must be 8 */
static struct {
	size_t availread;
	size_t pdata;
	size_t availwrite;
	size_t pfree;
	rfifo_t rfifo;
} x[] = {
	{ 7,1,0,1, { 1,9 } },
	{ 7,1,1,0, { 1,8 } },
	{ 0,0,8,0, { 0,0 } },
	{ 0,1,7,1, { 1,1 } },
	{ 1,1,6,2, { 1,2 } },
	{ 2,0,6,2, { 0,2 } },
	{ 1,0,7,1, { 0,1 } },
	{ 8,0,0,0, { 0,8 } },
	{ 6,2,1,1, { 2,9 } },
	{ 1,7,7,0, { 7,8 } },
	{ -1 }

};

#include <assert.h>

static void rfifo_test() 
{
    rfifo_t buf;
    char cw = 0;
    char cr_prev = cw;
    size_t dw;
	/* for overflow */
	buf.ofs_beg = -RFIFO_BUFSZ;
	buf.ofs_end = -RFIFO_BUFSZ;
	for(; (int)buf.ofs_beg < RFIFO_BUFSZ*4;) {
        size_t avread = rfifo_availread(&buf);
        size_t avwrite = rfifo_availwrite(&buf);

        assert(avread<=RFIFO_BUFSZ);
        assert(avwrite<=RFIFO_BUFSZ);
        assert( (0 < avread+avwrite) && (avread+avwrite <= RFIFO_BUFSZ) );

        if (rand() < (RAND_MAX / 2)) {
			char *pdata;
            size_t toread = 0xFFFFFFFF & (rand()*(avread+1)/((long long)RAND_MAX+1));
            assert(toread <= avread);

			pdata = rfifo_pdata(&buf);
            for (dw = 0; dw < toread; dw++) {
                assert(pdata[dw] == cr_prev++);
            }

            rfifo_markread(&buf, toread);
        } else {
			char *pfree;
            size_t towrite = rand()*(avwrite+1)/((long long)RAND_MAX+1);
            assert(towrite <= avwrite);
			pfree = rfifo_pfree(&buf);
            for (dw = 0; dw < towrite; dw++) {
                pfree[dw] = cw++;
            }
            rfifo_markwrite(&buf, towrite);
        }
        
    }
}

int main()
{
	int i, j;

	/* for overflow */
	for (i = 0; x[i].availread != -1; i++) {
		x[i].rfifo.ofs_beg -= RFIFO_BUFSZ;
		x[i].rfifo.ofs_end -= RFIFO_BUFSZ;
	}

	for (j = 0; j < 2; j++) {
		for (i = 0; x[i].availread != -1; i++) {

			 availread = rfifo_availread(&x[i].rfifo);
			 pdata = rfifo_pdata(&x[i].rfifo) - x[i].rfifo.data;
			 availwrite = rfifo_availwrite(&x[i].rfifo);
			 pfree = rfifo_pfree(&x[i].rfifo) - x[i].rfifo.data;

			assert(availread == x[i].availread);
			assert(pdata == x[i].pdata);
			assert(availwrite == x[i].availwrite);
			assert(pfree == x[i].pfree);

			x[i].rfifo.ofs_beg += RFIFO_BUFSZ;
			x[i].rfifo.ofs_end += RFIFO_BUFSZ;
		}
	}
	rfifo_test();
	return 0;
}
#endif
