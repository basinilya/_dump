#include "mylogging.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <stdlib.h> /* for exit(3) */
#include <stdint.h>

#include "mylastheader.h"

static char *thopows[] = {
	""," thousand"," million"," billion"
};

static char *firstscore[] = {
	"\0","-one","-two","-three","-four","-five","-six","-seven","-eight","-nine","-ten","-eleven","-twelve","-thirteen","-fourteen","-fifteen","-sixteen","-seventeen","-eighteen","-nineteen"
};

static char *decs[] = {
	"","","twenty","thirty","forty","fifty","sixty","seventy","eighty","ninety"
};

static char * _99_helper(int n, char *s) {
	if (n < 20) {
		return s + sprintf(s, "%s", firstscore[n]+1);
	} else {
		return s + sprintf(s, "%s%s", decs[n / 10], firstscore[n % 10]);
	}
}

static char * _hundred_helper(int n, char *s) {
	int n99 = n % 100;
	n = n / 100;

	if (n != 0) {
		s = _99_helper(n, s);
		s += sprintf(s, "%s", " hundred");
		if (n99 != 0) {
			s += sprintf(s, "%s", " and ");
		}
	}
	s = _99_helper(n99, s);
	return s;
}

static char * _thousands_helper(int neg, char *s, int lev) {
	int mod = neg % 1000; /* divide neg by 1000 and keep the modulo */
	neg = neg / 1000;
	if (neg != 0) {
		s = _thousands_helper(neg, s, lev+1); /* write more significant digits */
		if (mod != 0) {
			*s++ = ' ';
		}
	}
	if (mod != 0) {
		s = _hundred_helper(-mod, s);
		s += sprintf(s, "%s", thopows[lev]);
	}
	return s;
}

/* like itoa, but to words */
static int itowords(int n, char *s) {
	char *s2;
	if (n >= 0) {
		n = -n;
		s2 = s;
	} else {
		s2 = s + sprintf(s, "%s", "minus ");
	}
	s2 = _thousands_helper(n, s2, 0);
	*s2 = '\0';
	return s2 - s;
}

static void humanizets(char *s, int hours, int minutes, int seconds) {
	char *s2 = s;
	if (hours != 0) {
		s2 += itowords(hours, s2);
		s2 += sprintf(s2, "%s", hours == 1 ? " hour" : " hours");
	}
	if (minutes != 0) {
		if (s2 != s) {
			s2 += sprintf(s2, "%s", ", ");
		}
		s2 += itowords(minutes, s2);
		s2 += sprintf(s2, "%s", minutes == 1 ? " minute" : " minutes");
	}
	if (seconds != 0) {
		if (s2 != s) {
			s2 += sprintf(s2, "%s", ", ");
		}
		s2 += itowords(seconds, s2);
		s2 += sprintf(s2, "%s", seconds == 1 ? " second" : " seconds");
	}
	if (s2 == s) {
		s2 += sprintf(s2, "%s", "zero seconds");
	}
}

static int failed = 0;

static void test1(int i, const char *s) {
	char buf[500];
	itowords(i, buf);
	if (0 != strcmp(s, buf)) {
		failed = 1;
		fprintf(stderr, "failed test1 for %d. Expected:\n\t%s\nactual:\n\t%.500s\n\n", i, s, buf);
	}
}

static void test2(int millis, const char *s) {
	char buf[500];
	int seconds = millis / 1000;
	int minutes = seconds / 60;
	int hours = minutes / 60;
	minutes = minutes % 60;
	seconds = seconds % 60;
	humanizets(buf, hours, minutes, seconds);
	if (0 != strcmp(s, buf)) {
		failed = 1;
		fprintf(stderr, "failed test1 for %d. Expected:\n\t%s\nactual:\n\t%.500s\n\n", millis, s, buf);
	}
}

static void testall() {
	test1(-1, "minus one");
	test1(1, "one");
	test1(10, "ten");
	test1(11, "eleven");
	test1(20, "twenty");
	test1(122, "one hundred and twenty-two");
	test1(3501, "three thousand five hundred and one");
	test1(100, "one hundred");
	test1(1000, "one thousand");
	test1(100000, "one hundred thousand");
	test1(1000000, "one million");
	test1(10000000, "ten million");
	test1(100000000, "one hundred million");
	test1(1000000000, "one billion");
	test1(111, "one hundred and eleven");
	test1(1111, "one thousand one hundred and eleven");
	test1(111111, "one hundred and eleven thousand one hundred and eleven");
	test1(1111111, "one million one hundred and eleven thousand one hundred and eleven");
	test1(11111111, "eleven million one hundred and eleven thousand one hundred and eleven");
	test1(111111111, "one hundred and eleven million one hundred and eleven thousand one hundred and eleven");
	test1(1111111111, "one billion one hundred and eleven million one hundred and eleven thousand one hundred and eleven");
	test1(123, "one hundred and twenty-three");
	test1(1234, "one thousand two hundred and thirty-four");
	test1(12345, "twelve thousand three hundred and forty-five");
	test1(123456, "one hundred and twenty-three thousand four hundred and fifty-six");
	test1(1234567, "one million two hundred and thirty-four thousand five hundred and sixty-seven");
	test1(12345678, "twelve million three hundred and forty-five thousand six hundred and seventy-eight");
	test1(123456789, "one hundred and twenty-three million four hundred and fifty-six thousand seven hundred and eighty-nine");
	test1(1234567890, "one billion two hundred and thirty-four million five hundred and sixty-seven thousand eight hundred and ninety");
	test1(1234000890, "one billion two hundred and thirty-four million eight hundred and ninety");

	test2(100, "zero seconds");
	test2(2500, "two seconds");
	test2(122500, "two minutes, two seconds");
	test2(3722500, "one hour, two minutes, two seconds");
	test2(113722500, "thirty-one hours, thirty-five minutes, twenty-two seconds");
	test2(3600000, "one hour");
	test2(3720000, "one hour, two minutes");

	if (failed)
		exit(failed);
}

static void play(long long pos) {
	// #define SOUND_PCM_WRITE_BITS		SNDCTL_DSP_SETFMT
	// #define SOUND_PCM_WRITE_RATE		SNDCTL_DSP_SPEED
	// #define SOUND_PCM_WRITE_CHANNELS	SNDCTL_DSP_CHANNELS
}

static void fillrand(unsigned char *buf, size_t sz) {
	int i = 0 , j = 0;
	intptr_t limit = 0;
	intptr_t x = 0;

	for (;sz != 0;) {
		sz--;

		if (limit < 255) {
			limit *= (unsigned int)RAND_MAX + 1;
			limit += RAND_MAX;
			x *= ((unsigned int)RAND_MAX) + 1;
			x += rand();
			i++;
		}

		buf[sz] = x % 256;
		x /= 256;
		limit /= 256;
	}
}

static unsigned char sambuf[16/8*22050/1]; /* 100ms */

int main(int argc, char *argv[]) {
	testall();
	{
		int handle;
		int channels = 1;
		int bits = 16;
		int rate = 22050;
		static const char filename[] = "/dev/dsp";
		handle = open(filename, O_WRONLY);
		if (-1 == handle) {
			pSysError(ERR, "open('" FMT_S "') failed", filename);
			return 1;
		}

		if ( ioctl(handle,  SOUND_PCM_WRITE_BITS, &bits) == -1 )
		{
			pSysError(ERR, "ioctl bits");
			return 1;
		}
		if ( ioctl(handle, SOUND_PCM_WRITE_CHANNELS,&channels) == -1 )
		{
			pSysError(ERR, "ioctl channels");
			return 1;
		}

		if (ioctl(handle, SOUND_PCM_WRITE_RATE,&rate) == -1 )
		{
			pSysError(ERR, "ioctl sample rate");
			return 1;
		}
		fillrand(sambuf, sizeof(sambuf));

		if (-1 == write(handle, sambuf, sizeof(sambuf))) {
			pSysError(ERR, "write() failed");
			return 1;
		}
	}
	return 0;
}
