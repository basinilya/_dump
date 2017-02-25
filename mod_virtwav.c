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

static int handle_caching(request_rec *r, apr_time_t actual_mtime, apr_off_t actual_fsize) {
	int rc;

	ap_set_accept_ranges(r);

	ap_set_content_length(r, actual_fsize);

	ap_update_mtime(r, actual_mtime);
	ap_set_last_modified(r);

	ap_set_etag(r);

	rc = ap_meets_conditions(r);
	// Calls many ap_condition_* functions, including ap_condition_if_range()
	// may return OK, HTTP_NOT_MODIFIED or HTTP_PRECONDITION_FAILED
	// If If-Range present, returns OK regardless of the check
	
	if (rc != OK) return rc;
	
	// Not sure should I suppress the apache byterange filter this way:
	if (ap_condition_if_range(r) == AP_CONDITION_NOMATCH) {
		apr_table_unset(r->headers_in, "If-Range");
		apr_table_unset(r->headers_in, "Range");
		r->range = NULL;
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

	if (!r->handler || strcmp(r->handler, "virtwav-handler")) return (DECLINED);

	// read real file info
	{
		apr_finfo_t finfo;
		apr_stat(&finfo, r->filename, APR_FINFO_SIZE, r->pool);
		actual_fsize = finfo.size;
	}

	rc = handle_caching(r, ACTUAL_MTIME, actual_fsize);
	if (rc != OK) return rc;

	{
		apr_file_t fd;
		apr_off_t offset = 0;
		apr_size_t length = 0;
		apr_size_t nbytes;

		apr_file_open(&fd, r->filename,
			APR_FOPEN_READ | APR_FOPEN_BINARY | APR_FOPEN_SENDFILE_ENABLED | APR_FOPEN_LARGEFILE, APR_FPROT_OS_DEFAULT,
			r->pool);

		apr_file_seek(&fd, APR_SET, &offset);

		ap_send_fd(&fd, r, offset, apr_size_t length, &nbytes);
	}

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
