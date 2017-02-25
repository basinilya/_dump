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

static int foreach_range(void (*callback)(void *param), const char *range, apr_off_t actual_fsize) {
	apr_int64_t range_beg = 0, range_end;
	int nflds, nchars = -1;
	
	if (0 != strncasecmp(WORD_BYTES, range, sizeof(WORD_BYTES)-1)) {
		printf("bad Range: %s\n", range);
		return HTTP_NOT_FOUND;
	}
	range += sizeof(WORD_BYTES) - 2;

	for (;;) {
		//ap_rprintf(r, "%s\n", range);
		nflds = sscanf(range, "%*c%" APR_INT64_T_FMT "%n-%" APR_INT64_T_FMT "%n", &range_beg, &nchars, &range_end, &nchars);
		if (nflds <= 0) { // EOF or bad format
			break;
		} else if (nflds == 1) { // one number
			range_end = actual_fsize - 1;
		}
		if (range_beg < 0) { // relative to EOF
			range_beg += actual_fsize;
		}
		if (range_end < 0) { // relative to EOF
			range_end += actual_fsize;
		}
		printf("nflds:%d,nchars:%d: %" APR_INT64_T_FMT " %" APR_INT64_T_FMT "\n", nflds, nchars, range_beg, range_end);
		range += nchars;
	}
	return OK;
}

int main(int argc, char *argv[]) {
	const char *range = "bytes: 1-2, -1, 22-33";
	apr_off_t actual_fsize = 1000;
	return foreach_range(NULL, range, actual_fsize);
}

