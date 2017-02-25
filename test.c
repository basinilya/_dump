#include "apr_hash.h"
#include "ap_config.h"
#include "ap_provider.h"
#include "httpd.h"
#include "http_core.h"
#include "http_config.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"

#include <unistd.h> /* for sleep() */
#include <stdio.h>


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
	struct elem *first;
};

static int process_range_end(struct head *phead) {
	struct elem *pelem;
	printf("\ntotal bytes: %" APR_INT64_T_FMT "\n", phead->totalbytes);
	for (pelem = phead->first; pelem; pelem = pelem->next) {
		printf("%" APR_INT64_T_FMT " %" APR_INT64_T_FMT "\n", pelem->range_beg, pelem->range_end);
	}
	return OK;
}

static int process_range_mid(const char *range, struct head *phead, struct elem **pprev) {
	int nflds, nchars = -1;
	struct elem elem;

	nflds = sscanf(range, "%*c%" APR_INT64_T_FMT "%n-%" APR_INT64_T_FMT "%n", &elem.range_beg, &nchars, &elem.range_end, &nchars);
	if (nflds <= 0) { // EOF or bad format
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
	printf("nflds:%d,nchars:%d: %" APR_INT64_T_FMT " %" APR_INT64_T_FMT "\n", nflds, nchars, elem.range_beg, elem.range_end);
	range += nchars;
	phead->totalbytes += elem.range_end - elem.range_beg + 1;
	*pprev = &elem;
	return process_range_mid(range, phead, &elem.next);
}

static int process_range(const char *range, request_rec *r, apr_off_t actual_fsize) {
	struct head head = { r, actual_fsize, 0 };

	if (0 != strncasecmp(WORD_BYTES, range, sizeof(WORD_BYTES)-1)) {
		printf("bad Range: %s\n", range);
		return HTTP_NOT_FOUND;
	}
	range += sizeof(WORD_BYTES) - 2;

	return process_range_mid(range, &head, &head.first);
}

int main(int argc, char *argv[]) {
	const char *range = "bytes: 1-2, -1, 22-33";
	apr_off_t actual_fsize = 1000;
	return process_range(range, NULL, actual_fsize);
}

