#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libavformat/avformat.h>

#ifdef __cplusplus
}
#endif

int main1(int argc, char* argv[])
{
	int ret;
	AVOutputFormat *ofmt;
	AVFormatContext *ctx;

	av_register_all();
	ofmt = av_guess_format("mp3", NULL, NULL);
	if (!ofmt) {
		fprintf(stderr, "mp3 muxer not working\n");
		exit(1);
	}
	ret = avformat_alloc_output_context2(&ctx, ofmt, NULL, "out.mp3");
	if (ret < 0) {
		char errbuf[200];
		av_make_error_string(errbuf, sizeof(errbuf), ret);
		fprintf(stderr, "%s\n", errbuf);
		exit(1);
	}
	//av_dump_format
	//avio_open2(
//	avformat_open_input
	//
	return 0;
}
