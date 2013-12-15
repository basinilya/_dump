#include <stdio.h>

extern "C" {

#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

}

//#include <map>
//#include <string>
//using namespace std;

#undef av_err2str
struct _av_err2str_buf {
	char buf[AV_ERROR_MAX_STRING_SIZE];
};

#define av_err2str(errnum) \
    av_make_error_string(_av_err2str_buf().buf, AV_ERROR_MAX_STRING_SIZE, errnum)

#define STREAM_DURATION 0.2
#define STREAM_SAMPLE_FMT AV_SAMPLE_FMT_S16 /* default sample_fmt */

static const char filename[] = "out.mp3";

struct Bue {

//map<string, int> allocs;
#define allocs_inc(name) //(allocs[name]++)
#define allocs_dec(name) //(allocs[name]--)

/* Add an output stream. */
AVStream *add_stream(AVFormatContext *oc, AVCodec **codec,
                            enum AVCodecID codec_id)
{
    const AVSampleFormat *psamfmt;
    AVCodecContext *c;
    AVStream *st;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        exit(1);
    }

    st = avformat_new_stream(oc, *codec);
    if (!st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    st->id = oc->nb_streams-1;
    c = st->codec;

    for (psamfmt = (*codec)->sample_fmts;; psamfmt++) {
        if (*psamfmt == STREAM_SAMPLE_FMT) {
            c->sample_fmt       = STREAM_SAMPLE_FMT;
            break;
        } else if (*psamfmt == -1) {
            c->sample_fmt       = (*codec)->sample_fmts[0];
            break;
        }
    }
    c->bit_rate    = 64000;
    c->sample_rate = 44100;
    c->channels    = 2;

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return st;
}

/**************************************************************/
/* audio output */

float t, tincr, tincr2;
uint8_t **src_samples_data;
int       src_samples_linesize;
int       src_nb_samples;

int max_dst_nb_samples;
uint8_t **dst_samples_data;
int       dst_samples_linesize;
int       dst_samples_size;

struct SwrContext *swr_ctx;

Bue() {
    swr_ctx = NULL;
}

void open_audio(AVFormatContext *oc, AVCodec *codec, AVStream *st)
{
    AVCodecContext *c;
    int ret;

    c = st->codec;

    /* open it */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
        exit(1);
    }

    /* init signal generator */
    t     = 0;
    tincr = (float)(2 * M_PI * 110.0 / c->sample_rate);
    /* increment frequency by 110 Hz per second */
    tincr2 = (float)(2 * M_PI * 110.0 / c->sample_rate / c->sample_rate);

    src_nb_samples = c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE ?
        10000 : c->frame_size;

    ret = av_samples_alloc_array_and_samples(&src_samples_data, &src_samples_linesize, c->channels,
                                             src_nb_samples, AV_SAMPLE_FMT_S16, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate source samples\n");
        exit(1);
    }

    /* compute the number of converted samples: buffering is avoided
     * ensuring that the output buffer will contain at least all the
     * converted input samples */
    max_dst_nb_samples = src_nb_samples;

    /* create resampler context */
    if (c->sample_fmt != AV_SAMPLE_FMT_S16) {
        swr_ctx = swr_alloc();
        if (!swr_ctx) {
            fprintf(stderr, "Could not allocate resampler context\n");
            exit(1);
        }

        /* set options */
        av_opt_set_int       (swr_ctx, "in_channel_count",   c->channels,       0);
        av_opt_set_int       (swr_ctx, "in_sample_rate",     c->sample_rate,    0);
        av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
        av_opt_set_int       (swr_ctx, "out_channel_count",  c->channels,       0);
        av_opt_set_int       (swr_ctx, "out_sample_rate",    c->sample_rate,    0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

        /* initialize the resampling context */
        if ((ret = swr_init(swr_ctx)) < 0) {
            fprintf(stderr, "Failed to initialize the resampling context\n");
            exit(1);
        }

    ret = av_samples_alloc_array_and_samples(&dst_samples_data, &dst_samples_linesize, c->channels,
                                             max_dst_nb_samples, c->sample_fmt, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate destination samples\n");
        exit(1);
    }
    } else {
        dst_samples_data = src_samples_data;
    }
    dst_samples_size = av_samples_get_buffer_size(NULL, c->channels, max_dst_nb_samples,
                                                  c->sample_fmt, 0);
}

/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
 * 'nb_channels' channels. */
void get_audio_frame(int16_t *samples, int frame_size, int nb_channels)
{
    int j, i, v;
    int16_t *q;

    q = samples;
    for (j = 0; j < frame_size; j++) {
        v = (int)(sin(t) * 10000);
        for (i = 0; i < nb_channels; i++)
            *q++ = v;
        t     += tincr;
        tincr += tincr2;
    }
}

void write_audio_frame(AVFormatContext *oc, AVStream *st)
{
    AVCodecContext *c;
    AVPacket pkt = { 0 }; // data and size must be 0;
    AVFrame *frame = av_frame_alloc();
    int got_packet, ret, dst_nb_samples;

    av_init_packet(&pkt);
    c = st->codec;

    get_audio_frame((int16_t *)src_samples_data[0], src_nb_samples, c->channels);

    /* convert samples from native format to destination codec format, using the resampler */
    if (swr_ctx) {
        /* compute destination number of samples */
        dst_nb_samples = av_rescale_rnd(swr_get_delay(swr_ctx, c->sample_rate) + src_nb_samples,
                                        c->sample_rate, c->sample_rate, AV_ROUND_UP);
        if (dst_nb_samples > max_dst_nb_samples) {
            av_free(dst_samples_data[0]);
            ret = av_samples_alloc(dst_samples_data, &dst_samples_linesize, c->channels,
                                   dst_nb_samples, c->sample_fmt, 0);
            if (ret < 0)
                exit(1);
            max_dst_nb_samples = dst_nb_samples;
            dst_samples_size = av_samples_get_buffer_size(NULL, c->channels, dst_nb_samples,
                                                          c->sample_fmt, 0);
        }

        /* convert to destination format */
        ret = swr_convert(swr_ctx,
                          dst_samples_data, dst_nb_samples,
                          (const uint8_t **)src_samples_data, src_nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            exit(1);
        }
    } else {
        dst_nb_samples = src_nb_samples;
    }

    frame->nb_samples = dst_nb_samples;
    avcodec_fill_audio_frame(frame, c->channels, c->sample_fmt,
                             dst_samples_data[0], dst_samples_size, 0);

    ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        fprintf(stderr, "Error encoding audio frame: %s\n", av_err2str(ret));
        exit(1);
    }

    if (!got_packet)
        goto freeframe;

    pkt.stream_index = st->index;

    /* Write the compressed frame to the media file. */
    ret = av_interleaved_write_frame(oc, &pkt);
    if (ret != 0) {
        fprintf(stderr, "Error while writing audio frame: %s\n",
                av_err2str(ret));
        exit(1);
    }
freeframe:
    av_frame_free(&frame);
}

void close_audio(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);
    if (dst_samples_data != src_samples_data) {


        av_free(dst_samples_data[0]);

        av_free(dst_samples_data);
    }
    av_free(src_samples_data[0]);
    av_free(src_samples_data);
}

/**************************************************************/
/* media file output */
int main()
{
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVStream *audio_st;
    AVCodec *audio_codec;
    double audio_time;
    int ret;

    /* Initialize libavcodec, and register all codecs and formats. */
    ret = avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    if (ret < 0) {
	    char errbuf[200];
	    av_make_error_string(errbuf, sizeof(errbuf), ret);
	    fprintf(stderr, "%s\n", errbuf);
	    exit(1);
    }
    fmt = oc->oformat;
    audio_st = add_stream(oc, &audio_codec, fmt->audio_codec);

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    open_audio(oc, audio_codec, audio_st);

    av_dump_format(oc, 0, filename, 1);

    ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
    if (ret < 0) {
        fprintf(stderr, "Could not open '%s': %s\n", filename,
                av_err2str(ret));
            return 1;
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file: %s\n",
                av_err2str(ret));
        return 1;
    }

    for (;;) {
        /* Compute current audio and video time. */
        audio_time = audio_st->pts.val * av_q2d(audio_st->time_base);

        if (audio_time >= STREAM_DURATION)
            break;

        /* write interleaved audio and video frames */
        write_audio_frame(oc, audio_st);
    }

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(oc);

    /* Close each codec. */
    close_audio(oc, audio_st);

    /* Close the output file. */
    avio_close(oc->pb);

    if (swr_ctx) {
        swr_free(&swr_ctx);
    }

    avformat_free_context(oc);
    return 0;
}
};

int main1(int argc, char **argv);

int main(int argc, char* argv[])
{
    //return main1(argc, argv);
    av_register_all();
    av_log_set_level(AV_LOG_QUIET);

    Bue *bue;

    bue = new Bue();
    bue->main();
    delete bue;
//return 0;
   if (0)
    for(int i = 0;; i++) {
    bue = new Bue();
    bue->main();
    delete bue;
    }

    return 0;
}
