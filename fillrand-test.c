#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h> /* for exit(3) */
#include <stdint.h>

/* ############################# */
#undef FILLRAND
#undef FILLRAND_UINTMAX_T
#undef FILLRAND_NO_BIT_DISPLACEMENT
#undef FILLRAND_RAND_MAX

#define FILLRAND fillrand32_31_disp
#define FILLRAND_UINTMAX_T uint32_t
#define FILLRAND_NO_BIT_DISPLACEMENT 0
#define FILLRAND_RAND_MAX 0x7FFFFFFF
#include "fillrand.inc.h"


/* ############################# */
#undef FILLRAND
#undef FILLRAND_UINTMAX_T
#undef FILLRAND_NO_BIT_DISPLACEMENT
#undef FILLRAND_RAND_MAX

#define FILLRAND fillrand32_31_nodisp
#define FILLRAND_UINTMAX_T uint32_t
#define FILLRAND_NO_BIT_DISPLACEMENT 1
#define FILLRAND_RAND_MAX 0x7FFFFFFF
#include "fillrand.inc.h"


/* ############################# */
#undef FILLRAND
#undef FILLRAND_UINTMAX_T
#undef FILLRAND_NO_BIT_DISPLACEMENT
#undef FILLRAND_RAND_MAX

#define FILLRAND fillrand32_15_disp
#define FILLRAND_UINTMAX_T uint32_t
#define FILLRAND_NO_BIT_DISPLACEMENT 0
#define FILLRAND_RAND_MAX 0x7FFF
#include "fillrand.inc.h"


/* ############################# */
#undef FILLRAND
#undef FILLRAND_UINTMAX_T
#undef FILLRAND_NO_BIT_DISPLACEMENT
#undef FILLRAND_RAND_MAX

#define FILLRAND fillrand32_15_nodisp
#define FILLRAND_UINTMAX_T uint32_t
#define FILLRAND_NO_BIT_DISPLACEMENT 1
#define FILLRAND_RAND_MAX 0x7FFF
#include "fillrand.inc.h"


/* ############################# */
#undef FILLRAND
#undef FILLRAND_UINTMAX_T
#undef FILLRAND_NO_BIT_DISPLACEMENT
#undef FILLRAND_RAND_MAX

#define FILLRAND fillrand64_31_disp
#define FILLRAND_UINTMAX_T uint64_t
#define FILLRAND_NO_BIT_DISPLACEMENT 0
#define FILLRAND_RAND_MAX 0x7FFFFFFF
#include "fillrand.inc.h"


/* ############################# */
#undef FILLRAND
#undef FILLRAND_UINTMAX_T
#undef FILLRAND_NO_BIT_DISPLACEMENT
#undef FILLRAND_RAND_MAX

#define FILLRAND fillrand64_31_nodisp
#define FILLRAND_UINTMAX_T uint64_t
#define FILLRAND_NO_BIT_DISPLACEMENT 1
#define FILLRAND_RAND_MAX 0x7FFFFFFF
#include "fillrand.inc.h"


/* ############################# */
#undef FILLRAND
#undef FILLRAND_UINTMAX_T
#undef FILLRAND_NO_BIT_DISPLACEMENT
#undef FILLRAND_RAND_MAX

#define FILLRAND fillrand64_15_disp
#define FILLRAND_UINTMAX_T uint64_t
#define FILLRAND_NO_BIT_DISPLACEMENT 0
#define FILLRAND_RAND_MAX 0x7FFF
#include "fillrand.inc.h"


/* ############################# */
#undef FILLRAND
#undef FILLRAND_UINTMAX_T
#undef FILLRAND_NO_BIT_DISPLACEMENT
#undef FILLRAND_RAND_MAX

#define FILLRAND fillrand64_15_nodisp
#define FILLRAND_UINTMAX_T uint64_t
#define FILLRAND_NO_BIT_DISPLACEMENT 1
#define FILLRAND_RAND_MAX 0x7FFF
#include "fillrand.inc.h"


void dumbfillrand(unsigned char *buf, size_t sz) {
	unsigned int r; /* for unsigned promotion */

	unsigned int rbmask = 0;

	for (;sz != 0;) {
		sz--;

		if (rbmask < 255) {
			r = rand();
			rbmask = RAND_MAX;
		}

		buf[sz] = r % 256;
		r /= 256;
		rbmask /= 256;
	}
}


static int nrepeats;

static unsigned char testbuf[20*1024*1024];

static size_t nelems = sizeof(testbuf);

typedef void (*fillrand_t)(unsigned char *, size_t);

static int test(fillrand_t func) {
	int i, j, ret = 0;
	for (i = 0; i < nrepeats; i++) {
		func(testbuf, nelems);
		for (j = 0; j < sizeof(testbuf); j++) ret += testbuf[j];
	}
	return ret;
}

int main(int argc, char *argv[]) {
	fillrand_t func;

	if (argc != 3) {
		fprintf(stderr, "usage: %s nrepeats funcname\n", argv[0]);
		return 1;
	}

	if (0 == strcmp(argv[2],"dumbfillrand"))                       func = dumbfillrand;
	else if (0 == strcmp(argv[2],"fillrand32_31_disp"))                       func = fillrand32_31_disp;
	else if (0 == strcmp(argv[2],"fillrand32_31_nodisp"))                     func = fillrand32_31_nodisp;
	else if (0 == strcmp(argv[2],"fillrand32_15_disp"))                       func = fillrand32_15_disp;
	else if (0 == strcmp(argv[2],"fillrand32_15_nodisp"))                     func = fillrand32_15_nodisp;
	else if (0 == strcmp(argv[2],"fillrand64_31_disp"))                       func = fillrand64_31_disp;
	else if (0 == strcmp(argv[2],"fillrand64_31_nodisp"))                     func = fillrand64_31_nodisp;
	else if (0 == strcmp(argv[2],"fillrand64_15_disp"))                       func = fillrand64_15_disp;
	else if (0 == strcmp(argv[2],"fillrand64_15_nodisp"))                     func = fillrand64_15_nodisp;
	else {
		nelems /= 2;

		     if (0 == strcmp(argv[2],"wfillrand32_31_disp"))                       func = (fillrand_t)wfillrand32_31_disp;
		else if (0 == strcmp(argv[2],"wfillrand32_31_nodisp"))                     func = (fillrand_t)wfillrand32_31_nodisp;
		else if (0 == strcmp(argv[2],"wfillrand32_15_disp"))                       func = (fillrand_t)wfillrand32_15_disp;
		else if (0 == strcmp(argv[2],"wfillrand32_15_nodisp"))                     func = (fillrand_t)wfillrand32_15_nodisp;
		else if (0 == strcmp(argv[2],"wfillrand64_31_disp"))                       func = (fillrand_t)wfillrand64_31_disp;
		else if (0 == strcmp(argv[2],"wfillrand64_31_nodisp"))                     func = (fillrand_t)wfillrand64_31_nodisp;
		else if (0 == strcmp(argv[2],"wfillrand64_15_disp"))                       func = (fillrand_t)wfillrand64_15_disp;
		else if (0 == strcmp(argv[2],"wfillrand64_15_nodisp"))                     func = (fillrand_t)wfillrand64_15_nodisp;
		else {
			fprintf(stderr, "unknown func %s\n", argv[2]);
			return 1;
		}
	}
	nrepeats = atoi(argv[1]);

	return test(func);
}
