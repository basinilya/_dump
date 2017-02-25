/*
#include "apr_hash.h"
#include "ap_config.h"
#include "ap_provider.h"
#include "httpd.h"
#include "http_core.h"
#include "http_config.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"
*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

struct request_rec;
typedef struct request_rec request_rec;

#define APR_INT64_T_FMT PRId64
typedef int64_t apr_int64_t, apr_off_t;

#define HTTP_NOT_FOUND 404
#define OK 0

#define WORD_BYTES "bytes:"

struct elem {
	struct elem *next;
	apr_int64_t range_beg;
	apr_int64_t range_end;
};

struct head {
	request_rec *r;
	apr_off_t actual_fsize;
	apr_int64_t totalbytes;
	const char *range;
	struct elem *first;
};

static int process_range_end(struct head *phead) {
	int i;
	struct elem *pelem;
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
	printf("number of elements: %d; elem size: %" PRIdPTR "; average consumption: %" PRIdPTR "\n", i, sizeof(struct elem), total / i);
	return OK;
}

static int process_range_mid(struct head * const phead, struct elem ** const next_in_prev) {
	int nflds, nchars = -1;
	struct elem elem;

	nflds = sscanf(phead->range, "%*c%" APR_INT64_T_FMT "%n-%" APR_INT64_T_FMT "%n", &elem.range_beg, &nchars, &elem.range_end, &nchars);
	if (nflds <= 0) { // EOF or bad format
		*next_in_prev = NULL;
		return process_range_end(phead);
	} else if (nflds == 1) { // one number
		elem.range_end = phead->actual_fsize - 1;
	}
	if (elem.range_beg < 0) { // relative to EOF
		elem.range_beg += phead->actual_fsize;
	}
	if (elem.range_end < 0) { // relative to EOF
		elem.range_end += phead->actual_fsize;
	}
	phead->totalbytes += elem.range_end - elem.range_beg + 1;
	phead->range += nchars;
	*next_in_prev = &elem;
	return process_range_mid(phead, &elem.next);
}

static int process_range(const char *range, request_rec *r, apr_off_t actual_fsize) {
	struct head head = { r, actual_fsize, 0 };

	if (0 != strncasecmp(WORD_BYTES, range, sizeof(WORD_BYTES)-1)) {
		printf("bad Range: %s\n", range);
		return HTTP_NOT_FOUND;
	}
	range += sizeof(WORD_BYTES) - 2;

	head.range = range;

	return process_range_mid(&head, &head.first);
}

int main(int argc, char *argv[]) {
	const char *range = "bytes: 1-2, -1, 22-33, -1, 22-33, -1, 22-33, 1-2, -1, 22-33, -1, 22-33, 1-2, -1, 22-33, -1, 22-33, 1-2, -1, 22-33, -1, 22-33, 1-2";
	apr_off_t actual_fsize = 1000;

	return process_range(range, NULL, actual_fsize);
}

