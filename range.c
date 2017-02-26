#include "myrange.h"
#include "httpd.h"
/*
#include "apr_hash.h"
#include "ap_config.h"
#include "ap_provider.h"
#include "http_core.h"
#include "http_config.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"
*/

#include <stdio.h>
// #include <inttypes.h>
#include <string.h>

/*
struct request_rec;
typedef struct request_rec request_rec;

#define APR_INT64_T_FMT PRId64
typedef int64_t apr_int64_t, apr_off_t;

#define HTTP_NOT_FOUND 404
#define OK 0
*/


#define NOINLINE __attribute__((noinline))

#if 0
extern NOINLINE int process_range_end(struct range_head * const phead)
{
	int i;
	struct range_elem *pelem;
	ssize_t total;
	char *prev = (char *)phead;
	//printf("\ntotal bytes: %" APR_INT64_T_FMT "\n", phead->totalbytes);
	for (i = 0, pelem = phead->first; pelem; pelem = pelem->next, i++) {
		printf("%" APR_INT64_T_FMT " %" APR_INT64_T_FMT "\n", pelem->range_beg, pelem->range_end);
		printf("\tcurrent element consumes %" PRIdPTR " bytes\n", prev - (char*)pelem);
		prev = (char *)pelem;
	}
	total = (char *)phead - (char*)&pelem;
	printf("\nall elements consume %" PRIdPTR " bytes\n", total);
	printf("number of elements: %d; elem size: %" PRIdPTR "; average consumption: %" PRIdPTR "\n", i, sizeof(struct range_elem), total / i);
	return OK;
}
#endif

static NOINLINE int parse_one_range(struct range_head * const phead, struct range_elem * const pelem) {
	int nflds, nchars;
	nflds = sscanf(phead->range, "%*c%" APR_INT64_T_FMT "%n-%" APR_INT64_T_FMT "%n", &pelem->range_beg, &nchars, &pelem->range_end, &nchars);
	phead->range += nchars;
	return nflds;
}

static NOINLINE int process_range_mid_real(struct range_head * const phead, struct range_elem * const pelem, struct range_elem ** const next_in_prev);

static NOINLINE int process_range_mid(struct range_head * const phead, struct range_elem ** const next_in_prev)
{
	struct range_elem elem;
	return process_range_mid_real(phead, &elem, next_in_prev);
}

static int process_range_mid_real(struct range_head * const phead, struct range_elem * const pelem, struct range_elem ** const next_in_prev)
{
	int nflds = parse_one_range(phead, pelem);

	if (nflds <= 0) { // EOF or bad format
		*next_in_prev = NULL;
		return process_range_end(phead);
	}

	if (nflds == 1) { // one number
		pelem->range_end = phead->actual_fsize - 1;
	}
	if (pelem->range_beg < 0) { // relative to EOF
		pelem->range_beg += phead->actual_fsize;
	}
	if (pelem->range_end < 0) { // relative to EOF
		pelem->range_end += phead->actual_fsize;
	}

	phead->totalparts++;
	phead->totalbytes += pelem->range_end - pelem->range_beg + 1;
	*next_in_prev = pelem;

	return process_range_mid(phead, &pelem->next);
}

#define WORD_BYTES "bytes:"

int process_range(const char *range, void *ctx, apr_off_t actual_fsize)
{
	struct range_head head = { NULL, ctx, actual_fsize, 0 };

	if (0 != strncasecmp(WORD_BYTES, range, sizeof(WORD_BYTES)-1)) {
		printf("bad Range: %s\n", range);
		return HTTP_NOT_FOUND;
	}
	range += sizeof(WORD_BYTES) - 2;

	head.range = range;

	return process_range_mid(&head, &head.first);
}

#if 0
int main(int argc, char *argv[]) {

	const char *fine = "fine";
	const char *range = "bytes: 1-2, -1, 22-33, -1, 22-33, -1, 22-33, 1-2, -1, 22-33, -1, 22-33, 1-2, -1, 22-33, -1, 22-33, 1-2, -1, 22-33, -1, 22-33, 1-2";
	apr_off_t actual_fsize = 1000;

	process_range(range, NULL, actual_fsize);

	printf("all %s\n", fine);
}
#endif
