#include "myrange.h"
#include "saytimespan.h"

#include "mod_core.h"
#include "apr_hash.h"
#include "apr_strings.h"
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
#define ACTUAL_FSIZE_WAV 0x7FFFFFFEL

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

struct ctx {
	request_rec *r;
};

static apr_status_t pumpfile(struct ctx *ctx, apr_off_t ofs, apr_int64_t range_end) {
	apr_status_t aprrc;
	apr_size_t nbytes;
	for (; ofs != range_end + 1;) {
		char buf[1024];

		nbytes = range_end - ofs + 1;
		if (nbytes > sizeof(buf)) nbytes = sizeof(buf);

		virtwav_read(buf, nbytes, ofs);

		ofs += nbytes;
		
		if (-1 == ap_rwrite(buf, nbytes, ctx->r)) {
			return APR_EGENERAL;
		}
	}
	return APR_SUCCESS;
}

static int example_handler(request_rec *r)
{
	int rc;
	apr_off_t actual_fsize = ACTUAL_FSIZE_WAV;
	struct ctx ctx = { r };
	const char *range;

	apr_status_t aprrc;

	if (!r->handler || strcmp(r->handler, "virtwav-handler")) return (DECLINED);

	rc = handle_caching(r, ACTUAL_MTIME, actual_fsize);
	if (rc != OK) return rc;

	range = apr_table_get(r->headers_in, "Range");
	if (range) {
		clear_range_headers(r);
		r->status = HTTP_PARTIAL_CONTENT;
		rc = process_range(range, &ctx, actual_fsize);
		if (rc == HTTP_OK) {
			r->status = HTTP_OK;
			rc = OK;
		}
	} else {
		apr_size_t nbytes;

		aprrc = pumpfile(&ctx, 0, actual_fsize-1);
		if (APR_SUCCESS != aprrc) return HTTP_NOT_FOUND;
		rc = OK;
	}

	return rc;
}

static apr_status_t seek_and_pump(struct ctx *ctx, struct range_elem *pelem) {
	apr_status_t aprrc;

	aprrc = pumpfile(ctx, pelem->range_beg, pelem->range_end);
	if (APR_SUCCESS != aprrc) return aprrc;

	return APR_SUCCESS;
}

int process_range_end(struct range_head * const phead)
{
	int i;
	apr_size_t nbytes;
	struct range_elem *pelem;
	char contenttype[200];
	struct ctx *ctx = (struct ctx*)phead->ctx;
	apr_status_t aprrc;
	request_rec *r = ctx->r;
	const char *oldctype = r->content_type;

	if (phead->totalparts == 0) return HTTP_NOT_FOUND;
	//ap_set_content_length(r, phead->totalbytes);
	//ap_set_content_length(r, 3);
	//r->clength = 0;

	pelem = phead->first;
#define CONTENT_RANGE_FMT "bytes %" APR_INT64_T_FMT "-%" APR_INT64_T_FMT "/%" APR_INT64_T_FMT

	if (phead->totalparts == 1) {
		char *content_range = apr_psprintf(r->pool, CONTENT_RANGE_FMT, pelem->range_beg, pelem->range_end, phead->actual_fsize);
		apr_table_set(r->headers_out, apr_pstrdup(r->pool, "Content-Range"), content_range);

		aprrc = seek_and_pump(ctx, pelem);
		if (APR_SUCCESS != aprrc) return HTTP_NOT_FOUND;
	} else {

		sprintf(contenttype, "multipart/byteranges; boundary=\"%s\"", ap_multipart_boundary);
		ap_set_content_type(r, apr_pstrdup(r->pool, contenttype));

		for (i = 0; pelem; pelem = pelem->next, i++) {

			ap_rprintf(r, "--%s\r\n", ap_multipart_boundary);

			if (oldctype) ap_rprintf(r, "Content-Type: %s\r\n", oldctype);
			ap_rprintf(r, "Content-Range: " CONTENT_RANGE_FMT "\r\n", pelem->range_beg, pelem->range_end, phead->actual_fsize);
			ap_rprintf(r, "\r\n");

			aprrc = seek_and_pump(ctx, pelem);
			if (APR_SUCCESS != aprrc) return HTTP_NOT_FOUND;

			ap_rprintf(r, "\r\n");
		}
		ap_rprintf(r, "--%s--\r\n", ap_multipart_boundary);
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

