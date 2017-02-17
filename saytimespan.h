#ifndef _SAYTIMESPAN_H
#define _SAYTIMESPAN_H

#include <stdio.h>
#include <stdint.h>


struct ch_riffhdr {
	unsigned char ChunkID [4];
	unsigned char ChunkSize     [4];
	unsigned char Format  [4];
};

struct ch_wavhdr_fmt {
	unsigned char Subchunk1ID   [4];
	unsigned char Subchunk1Size [4];
	unsigned char AudioFormat   [2];
	unsigned char NumChannels   [2];
	unsigned char SampleRate    [4];
	unsigned char ByteRate[4];
	unsigned char BlockAlign    [2];
	unsigned char BitsPerSample [2];
};

struct ch_wavhdr_data {
	unsigned char Subchunk2ID   [4];
	unsigned char Subchunk2Size [4];
};

struct ch_wavhdr {
	struct ch_riffhdr riffhdr;
	struct ch_wavhdr_fmt wavhdr_fmt;
	union {
		struct ch_wavhdr_data wavhdr_data_pcm;
	} u;
};


struct riffhdr {
	uint32_t ChunkID;
	uint32_t ChunkSize;
	uint32_t Format;
};

struct wavhdr_fmt {
	uint32_t Subchunk1ID;
	uint32_t Subchunk1Size;
	uint16_t AudioFormat;
	uint16_t NumChannels;
	uint32_t SampleRate;
	uint32_t ByteRate;
	uint16_t BlockAlign;
	uint16_t BitsPerSample;
};

struct wavhdr_data {
	uint32_t Subchunk2ID;
	uint32_t Subchunk2Size;
};

struct wavhdr {
	struct riffhdr riffhdr;
	struct wavhdr_fmt wavhdr_fmt;
	union {
		struct wavhdr_data wavhdr_data_pcm;
	} u;
};


struct samples_entry {
	char word[20];
	FILE *f;
	struct wavhdr wavhdr;
};

extern struct samples_entry saytimespan_samples[];
extern size_t saytimespan_samples_count;


#endif /* _SAYTIMESPAN_H */
