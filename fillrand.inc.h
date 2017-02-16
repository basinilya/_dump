/*
Fastest functions according to test results:

i686:
	fillrand32_31_disp (uint32_t, big RAND_MAX, displacement, uchar array)
	wfillrand32_31_disp (ditto, but ushort array)

x86_64:
	fillrand64_31_disp (ditto, but uint64_t)
	wfillrand64_31_disp
*/


/* some defaults */
#ifndef FILLRAND
#define FILLRAND fillrand
#endif
#ifndef FILLRAND_RAND_MAX
#define FILLRAND_RAND_MAX RAND_MAX
#endif
#ifndef FILLRAND_UINTMAX_T
#define FILLRAND_UINTMAX_T uintptr_t
#endif


/* include self once for wide version */
#if FILLRAND_UCHAR_MAX != 65535

# undef CONCAT_AB_
# undef CONCAT_AB

# define CONCAT_AB_(a, b) a##b
# define CONCAT_AB(a, b) CONCAT_AB_(a, b)

# define FILLRAND_FUNCPREF w
# define FILLRAND_UCHAR unsigned short
# define FILLRAND_UCHAR_MAX 65535
# include "fillrand.inc.h"

# undef FILLRAND_FUNCPREF
# undef FILLRAND_UCHAR
# undef FILLRAND_UCHAR_MAX

# define FILLRAND_FUNCPREF
# define FILLRAND_UCHAR unsigned char
# define FILLRAND_UCHAR_MAX 255

#endif





/* Extracts pseudo-random bits from the value returned by rand().
   It starts by putting the least significant bits to the buffer element, shifts right and repeats.
   When not enough random bits ramain, they're saved for future use and rand() called again.
   Most significant bits in the new value are not random and they're replaced with the saved bits.
   On 32-bit systems the compound value doesn't fit in uintptr_t and some of the saved bits are discarded.
   Lastly, the buffer is filled backwards.
 */
void CONCAT_AB(FILLRAND_FUNCPREF, FILLRAND) (FILLRAND_UCHAR *buf, size_t sz)
{
	unsigned int r; /* for unsigned promotion */

	FILLRAND_UINTMAX_T rbits = 0;
	FILLRAND_UINTMAX_T rbmask = 0;

	for (;sz != 0;) {

		if (rbmask < FILLRAND_UCHAR_MAX) {
			r = rand();
#if FILLRAND_NO_BIT_DISPLACEMENT
			rbits += r * (rbmask+1);
#else
			rbits *= ((unsigned int)FILLRAND_RAND_MAX) + 1;
			rbits += r;
#endif

			rbmask *= (unsigned int)FILLRAND_RAND_MAX + 1;
			rbmask += FILLRAND_RAND_MAX;
		}
#if FILLRAND_RAND_MAX <= FILLRAND_UCHAR_MAX
		else /* first rand() did not provide enough bits for buf[sz] */
#endif
		{
			sz--;

			buf[sz] = rbits % (FILLRAND_UCHAR_MAX+1);
			rbits /= (FILLRAND_UCHAR_MAX+1);
			rbmask /= (FILLRAND_UCHAR_MAX+1);
		}

	}
}

#undef FILLRAND_FUNCPREF
#undef FILLRAND_UCHAR
#undef FILLRAND_UCHAR_MAX

