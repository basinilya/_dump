#include "myrange.h"
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

static apr_status_t pumpfile(request_rec *r, apr_file_t *fd, apr_off_t ofs, apr_int64_t range_end) {
	apr_status_t aprrc;
	apr_size_t nbytes;
	for (; ofs != range_end + 1;) {
		char buf[1024];

		nbytes = range_end - ofs + 1;
		if (nbytes > sizeof(buf)) nbytes = sizeof(buf);

		aprrc = apr_file_read(fd, buf, &nbytes);
		if (APR_SUCCESS != aprrc) return HTTP_NOT_FOUND;

		ofs += nbytes;
		
		ap_rwrite(buf, nbytes, r);
	}
	return APR_SUCCESS;
}

struct ctx {
	request_rec *r;
	apr_file_t *fd;
};

static int example_handler(request_rec *r)
{
	int rc;
	apr_off_t actual_fsize = ACTUAL_FSIZE_WAV;
	struct ctx ctx = { r };
	const char *range;

	apr_status_t aprrc;

	if (!r->handler || strcmp(r->handler, "virtwav-handler")) return (DECLINED);

	// read real file info
	{
		apr_finfo_t finfo;

		aprrc = apr_file_open(&ctx.fd, r->filename,
			APR_FOPEN_READ | APR_FOPEN_BINARY | APR_FOPEN_SENDFILE_ENABLED, APR_FPROT_OS_DEFAULT,
			r->pool);
		if (APR_SUCCESS != aprrc) return HTTP_NOT_FOUND;

		aprrc = apr_file_info_get(&finfo, APR_FINFO_SIZE, ctx.fd);
		if (APR_SUCCESS != aprrc) return HTTP_NOT_FOUND;

		//apr_stat(&finfo, r->filename, APR_FINFO_SIZE, r->pool);
		actual_fsize = finfo.size;
	}

	rc = handle_caching(r, ACTUAL_MTIME, actual_fsize);
	if (rc != OK) return rc;

	range = apr_table_get(r->headers_in, "Range");
	if (0 && range) {
		//clear_range_headers(r);
		// r->status = HTTP_PARTIAL_CONTENT;
		// r->status_line = apr_pstrdup(r->pool, "HTTP/1.1 206 Partial Contentttt");
		rc = process_range(range, &ctx, actual_fsize);
	} else {
		apr_size_t nbytes;

		aprrc = pumpfile(r, ctx.fd, 0, actual_fsize-1);
		//aprrc = ap_send_fd(ctx.fd, r, 0, actual_fsize, &nbytes);
		if (APR_SUCCESS != aprrc) return HTTP_NOT_FOUND;
		rc = OK;
	}

	apr_file_close(ctx.fd);

	return rc;
}

int process_range_end(struct range_head * const phead)
{
	int i;
	apr_size_t nbytes;
	apr_off_t ofs;
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

	if (phead->totalparts == 1) {
		char content_range[200];
		sprintf(content_range, "bytes %" APR_INT64_T_FMT "-%" APR_INT64_T_FMT "/%" APR_INT64_T_FMT "", pelem->range_beg, pelem->range_end, phead->actual_fsize);
		apr_table_set(r->headers_out, apr_pstrdup(r->pool, "Content-Range"), apr_pstrdup(r->pool, content_range));

		pelem = phead->first;

		aprrc = ap_send_fd(ctx->fd, r, pelem->range_beg, pelem->range_end - pelem->range_beg + 1, &nbytes);
		if (APR_SUCCESS != aprrc) return HTTP_NOT_FOUND;
	} else {

		sprintf(contenttype, "multipart/byteranges; boundary=\"%s\"", ap_multipart_boundary);
		ap_set_content_type(r, apr_pstrdup(r->pool, contenttype));

		for (i = 0, pelem = phead->first; pelem; pelem = pelem->next, i++) {

			ap_rprintf(r, "-%s\r\n", ap_multipart_boundary);

			if (oldctype) ap_rprintf(r, "Content-Type: %s\r\n", oldctype);
			ap_rprintf(r, "Content-Range: bytes %" APR_INT64_T_FMT "-%" APR_INT64_T_FMT "/%" APR_INT64_T_FMT "\r\n", pelem->range_beg, pelem->range_end, phead->actual_fsize);
			ap_rprintf(r, "\r\n");


			ofs = pelem->range_beg;
			aprrc = apr_file_seek(ctx->fd, APR_SET, &ofs);
			if (APR_SUCCESS != aprrc) return HTTP_NOT_FOUND;

			aprrc = pumpfile(r, ctx->fd, ofs, pelem->range_end);
			if (APR_SUCCESS != aprrc) return HTTP_NOT_FOUND;

			ap_rprintf(r, "\r\n");
		}
		ap_rprintf(r, "-%s-\r\n", ap_multipart_boundary);
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

