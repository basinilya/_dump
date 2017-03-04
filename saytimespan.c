#include "saytimespan.h"

#include "mylogging.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h> /* for exit(3) */
#include <arpa/inet.h> /* for htonl() */
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <pthread.h>

#include "mylastheader.h"


#define ST_HTONL(hostlong) { ((hostlong) >> 24) & 0xFF,((hostlong) >> 16) & 0xFF,((hostlong) >> 8) & 0xFF,(hostlong) & 0xFF }
#define ST_HTOLEL(hostlong) { (hostlong) & 0xFF,((hostlong) >> 8) & 0xFF,((hostlong) >> 16) & 0xFF,((hostlong) >> 24) & 0xFF }
#define ST_HTOLES(hostshort) { (hostshort) & 0xFF, ((hostshort) >> 8) & 0xFF }

#define CC4_RIFF 0x52494646
#define CC4_WAVE 0x57415645
#define CC4_FMT_ 0x666d7420
#define CC4_DATA 0x64617461
#define CC4(n) ((const char *)&n)

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
int itowords(int n, char *s)
{
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

void humanizets(char *s, int hours, int minutes, int seconds)
{
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

void seconds2span(long long seconds, int *phours, int *pminutes, int *pseconds)
{
	long long minutes = seconds / 60;
	*phours = (int)(minutes / 60);
	*pminutes = (int)(minutes % 60);
	*pseconds = (int)(seconds % 60);
}


void wavhdr_validate(const struct wavhdr *pwavhdr)
{
	uint32_t hSubchunk2Size = myletohl(pwavhdr->u.wavhdr_data_pcm.Subchunk2Size);
	uint16_t hNumChannels = myletohs(pwavhdr->wavhdr_fmt.NumChannels);
	uint16_t hBitsPerSample = myletohs(pwavhdr->wavhdr_fmt.BitsPerSample);
	uint32_t hSampleRate = myletohl(pwavhdr->wavhdr_fmt.SampleRate);
	uint32_t hRiffchunksize = myletohl(pwavhdr->riffhdr.ChunkSize);

	log(DBG,
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
		, hRiffchunksize, hRiffchunksize
		, CC4(pwavhdr->riffhdr.Format)
		, CC4(pwavhdr->wavhdr_fmt.Subchunk1ID)
		, myletohs(pwavhdr->wavhdr_fmt.NumChannels)
		, myletohl(pwavhdr->wavhdr_fmt.SampleRate)
		, myletohs(pwavhdr->wavhdr_fmt.BitsPerSample)
		, CC4(pwavhdr->u.wavhdr_data_pcm.Subchunk2ID)
		, myletohl(pwavhdr->u.wavhdr_data_pcm.Subchunk2Size), myletohl(pwavhdr->u.wavhdr_data_pcm.Subchunk2Size)
		);

	if (
		pwavhdr->riffhdr.ChunkID != htonl(CC4_RIFF)
		|| hRiffchunksize < 36+hSubchunk2Size
		|| pwavhdr->riffhdr.Format != htonl(CC4_WAVE)


		|| pwavhdr->wavhdr_fmt.Subchunk1ID != htonl(CC4_FMT_)
		|| pwavhdr->wavhdr_fmt.Subchunk1Size != myhtolel(16)
		|| pwavhdr->wavhdr_fmt.AudioFormat != myhtoles(1)
		|| pwavhdr->wavhdr_fmt.NumChannels != myhtoles(1)
		|| pwavhdr->wavhdr_fmt.SampleRate != myhtolel(SAYTIMESPAN_SAMPLE_RATE)
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

const union _u_wavhdr virtwav_header = {
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
			.SampleRate= ST_HTOLEL(SAYTIMESPAN_SAMPLE_RATE),
			.ByteRate= ST_HTOLEL(SAYTIMESPAN_SAMPLE_RATE * 1 * 16/8),
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

static void fread_or_die(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	if (size*nmemb != fread(ptr, size,  nmemb, stream)) {
		if (feof(stream)) {
			log(ERR, "fread() failed: End Of File");
		} else {
			pSysError(ERR, "fread() failed");
		}
		exit(1);
	}
}

static pthread_mutex_t samples_lock = PTHREAD_MUTEX_INITIALIZER;

void samples_entry_init(struct samples_entry *found)
{
	FILE *f;
	char wavfile[sizeof(SAYTIMESPAN_SAMPLES)+20];
	sprintf(wavfile, SAYTIMESPAN_SAMPLES "/%s.wav", found->word);
	f = fopen(wavfile, "rb");
	if (!f) {
		pSysError(ERR, "fopen('" FMT_S "') failed", wavfile);
		exit(1);
	}
	fread_or_die(&found->wavhdr, 1,  sizeof(struct wavhdr), f);
	found->data_ofs = sizeof(struct wavhdr);
	if (found->wavhdr.wavhdr_fmt.Subchunk1Size == myhtolel(16)) {
		/* Subchunk1 looks fine; skip unknown subchunk2 */
		while(found->wavhdr.u.wavhdr_data_pcm.Subchunk2ID != htonl(CC4_DATA)) {
			int32_t hSubchunk2Size = myletohl(found->wavhdr.u.wavhdr_data_pcm.Subchunk2Size);
			log(DBG, "current pos %ld; skip subchunk %.4s of size %u", ftell(f), CC4(found->wavhdr.u.wavhdr_data_pcm.Subchunk2ID), hSubchunk2Size);
			fseek(f, hSubchunk2Size, SEEK_CUR);
			fread_or_die(&found->wavhdr.u.wavhdr_data_pcm, 1,  sizeof(struct ch_wavhdr_data), f);
			found->data_ofs += hSubchunk2Size + sizeof(struct ch_wavhdr_data);
		}
	}
	log(DBG, "data offset: %u (0x%08X)", found->data_ofs, found->data_ofs);
	wavhdr_validate(&found->wavhdr);
	found->f = f;
}

static ssize_t _fillword(char *buf, ssize_t bufsz, ssize_t bufofs, const char *word) {
	int (*compar)(const void *, const void *) = (int (*)(const void *, const void *))strcmp;
	struct samples_entry *found;

	ssize_t newbufofs, toread, ofs_read_beg;
	long toskip;

	size_t hSubchunk2Size;

	log(DBG, "fillword     (buf=%p, bufsz=%" PRIdPTR ", bufofs=%" PRIdPTR ", word='%s')", buf, bufsz, bufofs, word);

	found = (struct samples_entry*)bsearch(word, saytimespan_samples, saytimespan_samples_count, sizeof(struct samples_entry), compar);
	if (!found) {
		log(ERR, "sample not found: %s", word);
		exit(1);
	}
	pthread_mutex_lock(&samples_lock);
	if (!found->f) {
		samples_entry_init(found);
	}
	hSubchunk2Size = myletohl(found->wavhdr.u.wavhdr_data_pcm.Subchunk2Size);

	newbufofs = bufofs + hSubchunk2Size;
	if (newbufofs > 0) { /* have something to read */
		if (newbufofs > bufsz)
			newbufofs = bufsz;

		ofs_read_beg = bufofs > 0 ? bufofs : 0;
		toread = newbufofs - ofs_read_beg;
		if (toread < 0)
			toread = 0;

		toskip = -bufofs;
		if (toskip < 0)
			toskip = 0;
		toskip += found->data_ofs;

		log(DBG, "fillword:     fseek %ld '%s'", toskip, word);
		fseek(found->f, toskip, SEEK_SET);

		log(DBG, "fillword:     fread %" PRIdPTR " bytes to ofs %" PRIdPTR " '%s'", toread, ofs_read_beg, word);
		fread(buf + ofs_read_beg, 1, toread, found->f);
	}
	pthread_mutex_unlock(&samples_lock);

	return newbufofs;
}

static ssize_t _fillwords(char *buf, ssize_t bufsz, ssize_t bufofs, char *words) {
	char *token;
	char *save_ptr_tok;
	static const char delim[] = " -,";

	log(DBG, "fillwords(buf=%p, bufsz=%" PRIdPTR ", bufofs=%" PRIdPTR ", words='%s')", buf, bufsz, bufofs, words);

	token = strtok_r(words, delim, &save_ptr_tok);
	while(token != NULL) {
		bufofs = _fillword(buf, bufsz, bufofs, token);
		if (bufofs == bufsz)
			break;
		token = strtok_r(NULL, delim, &save_ptr_tok);
	}
	return bufofs;
}

void virtpcm_read(void *_buf, ssize_t bufsz, uint64_t virtofs)
{
	char *buf = (char *)_buf;
	ssize_t bufofs;

	log(DBG, "virtpcm_read(buf=%p, bufsz=%" PRIdPTR ", virtofs=%" PRIu64 ")", _buf, bufsz, virtofs);

	// round down to nearest phrase
	bufofs = virtofs % SAYTIMESPAN_BYTES_IN_PHRASE;
	bufofs = -bufofs;
	virtofs += bufofs;
	
	while (bufofs != bufsz) {
		uint64_t tmpvirtofs;
		char words[500];
		int hours, minutes, seconds;
		ssize_t ofs_gap_beg, silencesz;

		seconds2span(virtofs / (SAYTIMESPAN_SAMPLE_SIZE*SAYTIMESPAN_SAMPLE_RATE), &hours, &minutes, &seconds);
		humanizets(words, hours, minutes, seconds);

		ofs_gap_beg = _fillwords(buf, bufsz, bufofs, words);

		tmpvirtofs = virtofs + (ofs_gap_beg - bufofs);
		// round up to next
		virtofs = ((( (tmpvirtofs) + (SAYTIMESPAN_BYTES_IN_PHRASE) - 1) / (SAYTIMESPAN_BYTES_IN_PHRASE)) * (SAYTIMESPAN_BYTES_IN_PHRASE));

		bufofs = ofs_gap_beg + (virtofs - tmpvirtofs);

		if (bufofs > bufsz)
			bufofs = bufsz;

		if (bufofs > 0) {
			// write silence
			if (ofs_gap_beg < 0)
				ofs_gap_beg = 0;
			silencesz = bufofs - ofs_gap_beg;
			if (silencesz < 0)
				silencesz = 0;
			log(DBG, "virtwav_read: writing %" PRIdPTR " bytes of silence at offset %" PRIdPTR "", silencesz, ofs_gap_beg);
			memset(buf + ofs_gap_beg, 0, silencesz);
		}
	}
}

void virtwav_read(void *_buf, ssize_t bufsz, uint32_t virtofs)
{
	char *buf = (char *)_buf;

	log(DBG, "virtwav_read(buf=%p, bufsz=%" PRIdPTR ", virtofs=%" PRIu32 ")", _buf, bufsz, virtofs);

	// assume virtofs can be an odd number
	// bufsz can include wav header, many silence parts and many phrases
	if (virtofs < sizeof(struct wavhdr)) {
		ssize_t n;
		log(DBG, "virtwav_read: writing wav header");
		n = sizeof(struct wavhdr) - virtofs;
		if (n > bufsz)
			n = bufsz;
		memcpy(buf, ((char*)&virtwav_header.x) + virtofs, n);

		bufsz -= n;
		if (bufsz == 0)
			return;
		virtofs += n;
		buf += n;
	}
	virtofs -= sizeof(struct wavhdr); /* now raw offset */
	virtpcm_read(buf, bufsz, virtofs);
}


