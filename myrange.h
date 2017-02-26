#ifndef _MYRANGE_H
#define _MYRANGE_H

#include <apr.h>
// #include <httpd.h>

struct range_elem {
	struct range_elem *next;
	apr_int64_t range_beg;
	apr_int64_t range_end;
};

struct range_head {
	struct range_elem *first;
	void *ctx;
	apr_off_t actual_fsize;
	apr_int64_t totalbytes;
	const char *range;
	int totalparts;
};

int process_range(const char *range, void *ctx, apr_off_t actual_fsize);
int process_range_end(struct range_head * const phead);

#endif /* _MYRANGE_H */
