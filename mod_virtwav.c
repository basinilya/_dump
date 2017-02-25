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

#define ACTUAL_MTIME apr_time_from_sec(946728000) /* date -d "2000-01-01 12:00 UTC" +%s */
#define ACTUAL_FSIZE_WAV 0x7FFFFFFFL

static void clear_range_headers(request_rec *r) {
	apr_table_unset(r->headers_in, "If-Range");
	apr_table_unset(r->headers_in, "Range");
	r->range = NULL;
}

static int handle_caching(request_rec *r, apr_time_t actual_mtime, apr_off_t actual_fsize) {
	int rc;

	ap_set_content_length(r, actual_fsize);
	ap_set_accept_ranges(r);

	ap_update_mtime(r, actual_mtime);
	ap_set_last_modified(r);

	ap_set_etag(r);

	rc = ap_meets_conditions(r);
	// Calls many ap_condition_* functions, including ap_condition_if_range()
	// may return OK, HTTP_NOT_MODIFIED or HTTP_PRECONDITION_FAILED
	// If If-Range present, returns OK regardless of the check
	
	if (rc != OK) return rc;
	
	// Not sure should I suppress the apache byterange filter this way:
	if (ap_condition_if_range(r, r->headers_out) == AP_CONDITION_NOMATCH) {
		clear_range_headers(r);
	}

/*
	ap_rprintf(r, "clength: %" APR_INT64_T_FMT "\n", (apr_int64_t)r->clength);
	ap_rprintf(r, "mtime: %" APR_TIME_T_FMT "\n", r->mtime);
	ap_rwrite("dummy", 5, r);
	ap_rflush(r);
*/
	return OK;
}

static int example_handler(request_rec *r)
{
	int rc;
	apr_off_t actual_fsize = ACTUAL_FSIZE_WAV;
	apr_int64_t range_beg = 0, range_end;
	const char *range;

	apr_status_t aprrc;
	apr_file_t *fd;

	if (!r->handler || strcmp(r->handler, "virtwav-handler")) return (DECLINED);

	// read real file info
	{
		apr_finfo_t finfo;

		aprrc = apr_file_open(&fd, r->filename,
			APR_FOPEN_READ | APR_FOPEN_BINARY | APR_FOPEN_SENDFILE_ENABLED, APR_FPROT_OS_DEFAULT,
			r->pool);
		if (APR_SUCCESS != aprrc) return HTTP_NOT_FOUND;

		aprrc = apr_file_info_get(&finfo, APR_FINFO_SIZE, fd);
		if (APR_SUCCESS != aprrc) return HTTP_NOT_FOUND;

		//apr_stat(&finfo, r->filename, APR_FINFO_SIZE, r->pool);
		actual_fsize = finfo.size;
	}

	rc = handle_caching(r, ACTUAL_MTIME, actual_fsize);
	if (rc != OK) return rc;

	range_end = actual_fsize - 1;

	if (r->range) {
		// Range: bytes=0-0, 500-999
		if (2 != sscanf(r->range, "%*[^=]=%" APR_INT64_T_FMT "-%" APR_INT64_T_FMT "", &range_beg, &range_end)) {
			return HTTP_NOT_FOUND;
		}
	}

	{
		apr_size_t nbytes;

		//apr_file_seek(&fd, APR_SET, &offset);
		aprrc = ap_send_fd(fd, r, range_beg, range_end, &nbytes);
		if (APR_SUCCESS != aprrc) return HTTP_NOT_FOUND;

// "Content-Range: bytes %lu-%lu/%lu\r\n",
//             resp_body->range->first, resp_body->range->last,
//             resp_body->len);
 
	}
	clear_range_headers(r);

	return OK;
}

static void register_hooks(apr_pool_t *pool)
{
	/* Create a hook in the request handler, so we get called when a request arrives */
	ap_hook_handler(example_handler, NULL, NULL, APR_HOOK_LAST);

	// ap_hook_dirwalk_stat ?
	// This hook allows modules to handle/emulate the apr_stat()

	// ap_hook_map_to_storage ?
	// This hook allow modules to set the per_dir_config based on their own
}

module AP_MODULE_DECLARE_DATA   virtwav_module =
{
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	register_hooks   /* Our hook registering function */
};



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

