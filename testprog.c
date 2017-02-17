#include "saytimespan.h"

#include "mylogging.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <stdlib.h> /* for exit(3) */
#include <arpa/inet.h> /* for htonl() */
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

static void seconds2span(long long seconds, int *phours, int *pminutes, int *pseconds) {
	long long minutes = seconds / 60;
	*phours = (int)(minutes / 60);
	*pminutes = (int)(minutes % 60);
	*pseconds = (int)(seconds % 60);
}


#define ST_HTONL(hostlong) { ((hostlong) >> 24) & 0xFF,((hostlong) >> 16) & 0xFF,((hostlong) >> 8) & 0xFF,(hostlong) & 0xFF }
#define ST_HTOLEL(hostlong) { (hostlong) & 0xFF,((hostlong) >> 8) & 0xFF,((hostlong) >> 16) & 0xFF,((hostlong) >> 24) & 0xFF }
#define ST_HTOLES(hostshort) { (hostshort) & 0xFF, ((hostshort) >> 8) & 0xFF }

#define CC4_RIFF 0x52494646
#define CC4_WAVE 0x57415645
#define CC4_FMT_ 0x666d7420
#define CC4_DATA 0x64617461
#define CC4(n) ((const char *)&n)

#define SAMPLE_SIZE (16/8)
#define SAMPLE_RATE 22050

#include <errno.h>


/* ! Byte swap unsigned short */
static uint16_t swap_uint16( uint16_t val ) 
{
    return (val << 8) | (val >> 8 );
}

/* ! Byte swap unsigned int */
static uint32_t swap_uint32( uint32_t val )
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | (val >> 16);
}

#define myhtoles(x) swap_uint16(htons(x))
#define myletohs(x) ntohs(swap_uint16(x))

#define myhtolel(x) swap_uint32(htonl(x))
#define myletohl(x) ntohl(swap_uint32(x))


static void wavhdr_validate(const struct wavhdr *pwavhdr) {
	uint32_t hSubchunk2Size = myletohl(pwavhdr->u.wavhdr_data_pcm.Subchunk2Size);
	uint16_t hNumChannels = myletohs(pwavhdr->wavhdr_fmt.NumChannels);
	uint16_t hBitsPerSample = myletohs(pwavhdr->wavhdr_fmt.BitsPerSample);
	uint32_t hSampleRate = myletohl(pwavhdr->wavhdr_fmt.SampleRate);

	if (0) {
		printf(
				 "wavhdr.riffhdr.ChunkID:\t%.4s"
			"\n" "wavhdr.riffhdr.ChunkSize:\t%u (0x%08X)"
			"\n" "wavhdr.riffhdr.Format:\t%.4s"
			"\n" "wavhdr.wavhdr_fmt.Subchunk1ID:\t%.4s"
			"\n" "_"
			"\n" "wavhdr.wavhdr_fmt.NumChannels:\t%d"
			"\n" "wavhdr.wavhdr_fmt.SampleRate:\t%d"
			"\n" "wavhdr.wavhdr_fmt.BitsPerSample:\t%d"
			"\n" "_"
			"\n" "wavhdr.u.wavhdr_data_pcm.Subchunk2ID:\t%.4s"
			"\n" "wavhdr.u.wavhdr_data_pcm.Subchunk2Size:\t%u (0x%08X)"
			"\n" "_"
			"\n"
			, CC4(pwavhdr->riffhdr.ChunkID)
			, myletohl(pwavhdr->riffhdr.ChunkSize), myletohl(pwavhdr->riffhdr.ChunkSize)
			, CC4(pwavhdr->riffhdr.Format)
			, CC4(pwavhdr->wavhdr_fmt.Subchunk1ID)
			, myletohs(pwavhdr->wavhdr_fmt.NumChannels)
			, myletohl(pwavhdr->wavhdr_fmt.SampleRate)
			, myletohs(pwavhdr->wavhdr_fmt.BitsPerSample)
			, CC4(pwavhdr->u.wavhdr_data_pcm.Subchunk2ID)
			, myletohl(pwavhdr->u.wavhdr_data_pcm.Subchunk2Size), myletohl(pwavhdr->u.wavhdr_data_pcm.Subchunk2Size)
			);
	}

	if (
		pwavhdr->riffhdr.ChunkID != htonl(CC4_RIFF)
		|| pwavhdr->riffhdr.ChunkSize != myhtolel(36+hSubchunk2Size)
		|| pwavhdr->riffhdr.Format != htonl(CC4_WAVE)


		|| pwavhdr->wavhdr_fmt.Subchunk1ID != htonl(CC4_FMT_)
		|| pwavhdr->wavhdr_fmt.Subchunk1Size != myhtolel(16)
		|| pwavhdr->wavhdr_fmt.AudioFormat != myhtoles(1)
		|| pwavhdr->wavhdr_fmt.NumChannels != myhtoles(1)
		|| pwavhdr->wavhdr_fmt.SampleRate != myhtolel(SAMPLE_RATE)
		|| pwavhdr->wavhdr_fmt.ByteRate != myhtolel(hSampleRate * hNumChannels * hBitsPerSample/8)
		|| pwavhdr->wavhdr_fmt.BlockAlign != myhtolel(hNumChannels * hBitsPerSample/8)
		|| pwavhdr->wavhdr_fmt.BitsPerSample != myhtoles(16)

		|| pwavhdr->u.wavhdr_data_pcm.Subchunk2ID != htonl(CC4_DATA)
/*
*/     
		)
	{
		log(ERR, "bad wav header");
		exit(1);
	}
}


static const union {
	struct ch_wavhdr ch;
	struct wavhdr x;
} virtwav_header = {
	.ch = {
		.riffhdr = {
			.ChunkID = ST_HTONL(CC4_RIFF),
			.ChunkSize = ST_HTOLEL(0x7FFFFFFE),
			.Format = ST_HTONL(CC4_WAVE)
		},
		.wavhdr_fmt = {
			.Subchunk1ID= ST_HTONL(CC4_FMT_),
			.Subchunk1Size= ST_HTOLEL(16),
			.AudioFormat= ST_HTOLES(1),
			.NumChannels= ST_HTOLES(1),
			.SampleRate= ST_HTOLEL(SAMPLE_RATE),
			.ByteRate= ST_HTOLEL(SAMPLE_RATE * 1 * 16/8),
			.BlockAlign= ST_HTOLES(1 * 16/8),
			.BitsPerSample= ST_HTOLES(16)
		},
		.u = {
			.wavhdr_data_pcm = {
				.Subchunk2ID = ST_HTONL(CC4_DATA),
				.Subchunk2Size = ST_HTOLEL(0x7FFFFFFE - 36),
			}
		}
	}
};



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
	int rate = SAMPLE_RATE * 2;
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

static void samples_entry_init(struct samples_entry *found) {
	FILE *f;
	char wavfile[40];
	sprintf(wavfile, "samples/%s.wav", found->word);
	found->f = f = fopen(wavfile, "rb");
	if (!f) {
		pSysError(ERR, "fopen('" FMT_S "') failed", wavfile);
		exit(1);
	}
	if (sizeof(struct wavhdr) != fread(&found->wavhdr, 1,  sizeof(struct wavhdr), f)) {
		if (feof(f)) {
			log(ERR, "fread() failed: End Of File");
		} else {
			pSysError(ERR, "fread() failed");
		}
		exit(1);
	}
	wavhdr_validate(&found->wavhdr);
}

static ssize_t _virtwav_fillwords(unsigned char *buf, size_t bufsz, ssize_t bufofs, char *words) {
	ssize_t structremain;
	char *token;
	char *save_ptr_tok;
	static const char delim[] = " -,";
	struct samples_entry *found;
	long fileofs = sizeof(struct wavhdr) - bufofs;
	int (*compar)(const void *, const void *) = (int (*)(const void *, const void *))strcmp;

	log(INFO, "fillwords(buf=%p, bufsz=%u, bufofs=%d, words='%s')", buf, bufsz, bufofs, words);

	token = strtok_r(words, delim, &save_ptr_tok);
	while(token != NULL) {
		uint32_t hSubchunk2Size;
		log(INFO, "fillwords:     buf=%p, bufsz=%u, bufofs=%d, token='%s'", buf, bufsz, bufofs, token);
		found = (struct samples_entry*)bsearch(token, saytimespan_samples, saytimespan_samples_count, sizeof(struct samples_entry), compar);
		if (!found) {
			log(ERR, "sample not found: %s", token);
			exit(1);
		}
		if (!found->f) {
			samples_entry_init(found);
		}
		hSubchunk2Size = myletohl(found->wavhdr.u.wavhdr_data_pcm.Subchunk2Size);

		structremain = hSubchunk2Size + bufofs;
		if (structremain > 0) {
			/* not skip */
			log(INFO, "fillwords:     fseek %d '%s'", fileofs, token);
			fseek(found->f, fileofs, SEEK_SET);
			
			if (structremain > bufsz)
				structremain = bufsz;
			log(INFO, "fillwords:     copying %d bytes of wav data from offset %d to %p '%s'", structremain, -bufofs, buf, token);
			fread(buf, 1, structremain, found->f);

			bufsz -= structremain;
			buf += structremain;
		}
		bufofs += structremain;
		if (bufsz == 0)
			break;
		//
		fileofs = sizeof(struct wavhdr);
		token = strtok_r(NULL, delim, &save_ptr_tok);
	}
	return bufofs;
}

static void virtwav_read(void *_buf, uint32_t virtofs, size_t count) {
	unsigned char *buf = (unsigned char *)_buf;
	uint32_t saying_start_ofs;
	ssize_t n;

	log(INFO, "virtwav_read(buf=%p, virtofs=%u, count=%u)", _buf, virtofs, count);

	// assume virtofs can be an odd number
	// count can include wav header, many silence parts and many sayings
	if (virtofs < sizeof(struct wavhdr)) {
		log(INFO, "virtwav_read: writing wav header");
		n = sizeof(struct wavhdr) - virtofs;
		if (n > count)
			n = count;
		memcpy(buf, ((char*)&virtwav_header.x) + virtofs, n);

		count -= n;
		if (count == 0)
			return;
		virtofs += n;
		buf += n;
	}
	virtofs -= sizeof(struct wavhdr); /* now raw offset */

#define BYTES_IN_SAYING (SAMPLE_SIZE*SAMPLE_RATE*20)
		// round down to nearest saying
		saying_start_ofs = virtofs - virtofs % BYTES_IN_SAYING;

	
	while (count != 0) {
		char words[500];
		int hours, minutes, seconds;

		seconds2span(saying_start_ofs / (SAMPLE_SIZE*SAMPLE_RATE), &hours, &minutes, &seconds);
		humanizets(words, hours, minutes, seconds);

		n = _virtwav_fillwords(buf, count, saying_start_ofs - virtofs, words);
		if (n > 0) {
			virtofs += n;
			buf += n;
			count -= n;
		}

		log(INFO, "virtwav_read: count: %d, n: %d", count, n);
		if ((int)count < 0) {
			log(ERR, "program error");
			exit(1);
		}

		// round up to next
		saying_start_ofs = ((( (virtofs) + (BYTES_IN_SAYING) - 1) / (BYTES_IN_SAYING)) * (BYTES_IN_SAYING));

		// fill space before next saying with silence
		n = saying_start_ofs - virtofs;
		if (n > count)
			n = count;

		if (n != 0) {
			log(INFO, "virtwav_read: writing %d bytes of silence at %p", n, buf);
			memset(buf, 0, n);
			virtofs += n;
			buf += n;
			count -= n;
		}
	}
}

static void play(long long pos) {
	char buf[500];
	/*
	humanizets(buf, hours, minutes, seconds);

	saytimespan_samples
	*/
}



#define BUFSAMPLES (SAMPLE_RATE * 60)
static unsigned short sambuf[SAMPLE_SIZE*BUFSAMPLES/sizeof(short)];

int main(int argc, char *argv[]) {
	FILE *f;

	testall();
	//exit(0);
	{
		ossfd = init_oss();
		if (-1 == ossfd) {
			return 1;
		}
		virtwav_read(&sambuf, sizeof(struct wavhdr) + SAMPLE_SIZE*SAMPLE_RATE*20, sizeof(sambuf));

		if (-1 == write(ossfd, sambuf, sizeof(sambuf))) {
			pSysError(ERR, "write() failed");
			return 1;
		}

		play(0);
		for (;0;) {
			//wfillrand(sambuf, sizeof(sambuf)/sizeof(short));

		}
	}
	return 0;
}
