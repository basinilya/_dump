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
   It starts by putting the least significant octet to the buffer, shifts right and repeats.
   When less than 8 random bits ramain, they're saved for future use and rand() called again.
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
		sz--;

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

		buf[sz] = rbits % (FILLRAND_UCHAR_MAX+1);
		rbits /= (FILLRAND_UCHAR_MAX+1);
		rbmask /= (FILLRAND_UCHAR_MAX+1);
	}
}

#undef FILLRAND_FUNCPREF
#undef FILLRAND_UCHAR
#undef FILLRAND_UCHAR_MAX

