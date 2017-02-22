#include "saytimespan.h"

#include "mylogging.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h> /* for O_WRONLY */
#include <sys/soundcard.h>
#include <stdlib.h> /* for exit(3) */
#include <arpa/inet.h> /* for htonl() */
#include <stdint.h>
#include <sys/ioctl.h> /* for ioctl() */
#include <unistd.h> /* for write() */
#include <inttypes.h>
#include <errno.h>
#include <limits.h>

#include "mylastheader.h"

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
	int hours, minutes, seconds;
	seconds2span(millis / 1000, &hours, &minutes, &seconds);
	humanizets(buf, hours, minutes, seconds);
	if (0 != strcmp(s, buf)) {
		failed = 1;
		fprintf(stderr, "failed test1 for %d. Expected:\n\t%s\nactual:\n\t%.500s\n\n", millis, s, buf);
	}
}

#include "fillrand.inc.h"

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

	wavhdr_validate(&virtwav_header.x);

	if (failed)
		exit(failed);
}


static int init_oss() {
	int ossfd;
	int channels = 1;
	int bits = 16;
	int rate = SAYTIMESPAN_SAMPLE_RATE;
	static const char filename[] = "/dev/dsp";
	ossfd = open(filename, O_WRONLY);
	if (-1 == ossfd) {
		pSysError(ERR, "open('" FMT_S "') failed", filename);
		return -1;
	}

	if ( ioctl(ossfd,  SOUND_PCM_WRITE_BITS, &bits) == -1 )
	{
		pSysError(ERR, "ioctl bits");
		return -1;
	}
	if ( ioctl(ossfd, SOUND_PCM_WRITE_CHANNELS,&channels) == -1 )
	{
		pSysError(ERR, "ioctl channels");
		return -1;
	}

	if (ioctl(ossfd, SOUND_PCM_WRITE_RATE,&rate) == -1 )
	{
		pSysError(ERR, "ioctl sample rate");
		return -1;
	}
	return ossfd;
}

static int ossfd;

#define BUFSAMPLES (SAYTIMESPAN_SAMPLE_RATE * 20)
static unsigned short sambuf[SAYTIMESPAN_SAMPLE_SIZE*BUFSAMPLES/sizeof(short)];

int main(int argc, char *argv[]) {
	testall();
	//exit(0);
	{
		ossfd = init_oss();
		if (-1 == ossfd) {
			return 1;
		}
		if (0) {
			int i, testbufsz;
			for(i = 0; i < 2; i++) {
				for (testbufsz = 1024*1024; testbufsz > 0; testbufsz = (testbufsz/2-1) ) {
					char testbuf[testbufsz];
					char testname[40];
					FILE *testf;
					int pos = 0;
					int wrbytes;
					sprintf(testname, "test%d-%d.wav", i, testbufsz);
					printf("begin %s\n", testname);
					testf = fopen(testname, "wb");
				
					for(;pos != -1;) {
						fillrand(testbuf, testbufsz);
						virtwav_read(testbuf, testbufsz, pos);
						fseek(testf, pos, SEEK_SET);

						wrbytes = testbufsz - i;
						if (wrbytes <= 0)
							wrbytes = 1;

						pos += wrbytes;

						if (pos > 1024*1024) {
							wrbytes -= (pos - (1024*1024));
							pos = -1;
						}

						fwrite(testbuf, 1, wrbytes, testf);

					}
				
					fclose(testf);
					printf("end %s\n", testname);
				}
			}
		}
		if (1) {
			wfillrand(sambuf, sizeof(sambuf)/sizeof(short));
			virtpcm_read(&sambuf, sizeof(sambuf), SAYTIMESPAN_SAMPLE_SIZE*SAYTIMESPAN_SAMPLE_RATE * (60ULL*60*INT_MAX + 60*35 + 40));

			if (-1 == write(ossfd, sambuf, sizeof(sambuf))) {
				pSysError(ERR, "write() failed");
				return 1;
			}
		}

		for (;0;) {
			//

		}
	}
	return 0;
}
