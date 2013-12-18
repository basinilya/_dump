#include <stdio.h>

extern "C" {

#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

}

#undef av_err2str
struct _av_err2str_buf {
	char buf[AV_ERROR_MAX_STRING_SIZE];
};

#define av_err2str(errnum) \
    av_make_error_string(_av_err2str_buf().buf, AV_ERROR_MAX_STRING_SIZE, errnum)

#define STREAM_SAMPLE_FMT AV_SAMPLE_FMT_S16 /* default sample_fmt */

struct Bue {

/* Add an output stream. */
static AVStream *add_stream(AVFormatContext *oc, AVCodec **codec,
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
    if (c->bit_rate == 0) c->bit_rate    = 64000;
    c->sample_rate = 48000;
    c->channels    = 2;

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= CODEC_FLAG_GLOBAL_HEADER;

    return st;
}

/**************************************************************/
/* audio output */

int dst_nb_samples;
uint8_t **dst_samples_data;
int       dst_samples_size;

struct SwrContext *swr_ctx;
AVFrame *shared_frame;

AVStream *audio_st;
AVFormatContext *output_ctx;

void open_audio()
{
    AVCodecContext *c;
    AVCodec *audio_codec;
    AVFormatContext *oc = output_ctx;
    AVOutputFormat *fmt = oc->oformat;
    int ret;

    audio_st = add_stream(output_ctx, &audio_codec, fmt->audio_codec);
    c = audio_st->codec;

    /* open it */
    ret = avcodec_open2(c, audio_codec, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
        exit(1);
    }

    dst_nb_samples = c->codec->capabilities & CODEC_CAP_VARIABLE_FRAME_SIZE ?
        10000 : c->frame_size;

    shared_frame = av_frame_alloc();
    if (!shared_frame){
        fprintf(stderr, "Could not allocate frame\n");
        exit(1);
    }

    /* create resampler context */
    swr_ctx = swr_alloc();
    if (!swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        exit(1);
    }

    /* set options */
    av_opt_set_int       (swr_ctx, "in_channel_count",   2,                 0);
    av_opt_set_int       (swr_ctx, "in_sample_rate",     48000,             0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
    av_opt_set_int       (swr_ctx, "out_channel_count",  c->channels,       0);
    av_opt_set_int       (swr_ctx, "out_sample_rate",    c->sample_rate,    0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

    /* initialize the resampling context */
    if ((ret = swr_init(swr_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        exit(1);
    }

    ret = av_samples_alloc_array_and_samples(&dst_samples_data, NULL, c->channels,
                                             dst_nb_samples, c->sample_fmt, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate destination samples\n");
        exit(1);
    }
    dst_samples_size = av_samples_get_buffer_size(NULL, c->channels, dst_nb_samples,
                                                  c->sample_fmt, 0);
}

void open(const char *filename) {
    int ret;

    /* Initialize libavcodec, and register all codecs and formats. */
    ret = avformat_alloc_output_context2(&output_ctx, NULL, NULL, filename);
    if (ret < 0) {
	    fprintf(stderr, "%s\n", av_err2str(ret));
	    exit(1);
    }

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    open_audio();

    av_dump_format(output_ctx, 0, filename, 1);

    ret = avio_open(&output_ctx->pb, filename, AVIO_FLAG_WRITE);
    if (ret < 0) {
        fprintf(stderr, "Could not open '%s': %s\n", filename,
                av_err2str(ret));
            exit(1);
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(output_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file: %s\n",
                av_err2str(ret));
        exit(1);
    }
}

void encode_and_write_frame(AVFrame *frame)
{
    AVCodecContext *c = audio_st->codec;
    int got_packet;
    AVPacket pkt = { 0 }; // data and size must be 0;
    int ret;

    av_init_packet(&pkt);
    ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        fprintf(stderr, "Error encoding audio frame: %s\n", av_err2str(ret));
        exit(1);
    }

    if (got_packet) {
        pkt.stream_index = audio_st->index;

        /* Write the compressed frame to the media file. */
        ret = av_interleaved_write_frame(output_ctx, &pkt);
        if (ret != 0) {
            fprintf(stderr, "Error while writing audio frame: %s\n",
                    av_err2str(ret));
            exit(1);
        }
    }
}

void encode_and_write(uint8_t *s16_samples, int nb_samples)
{
    AVCodecContext *c = audio_st->codec;
    int64_t nb_samples_total;
    uint8_t *s16_samples_ptr_arr_[1];
    uint8_t **s16_samples_ptr_arr = NULL;
    int ret;

    if (s16_samples) {
        s16_samples_ptr_arr_[0] = s16_samples;
        s16_samples_ptr_arr = s16_samples_ptr_arr_;
    }

    /* Estimate nb_samples after appending to buffered.
     * Using AV_ROUND_DOWN, because better estimate less and get real number on
     * next iteration than estimate more and get half-empty frame */
    nb_samples_total = swr_get_delay(swr_ctx, c->sample_rate)
            + av_rescale_rnd(nb_samples, c->sample_rate, 48000, AV_ROUND_DOWN);

    for(;;) {
        AVFrame *frame = NULL;
        int out_count = dst_nb_samples;

        if (nb_samples_total < dst_nb_samples && s16_samples != NULL) out_count = 0;

        ret = swr_convert(swr_ctx,
                          dst_samples_data, out_count,
                          (const uint8_t **)s16_samples_ptr_arr, nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            exit(1);
        }

        if (ret > 0) {
            frame = shared_frame;

            frame->nb_samples = ret;
            avcodec_fill_audio_frame(frame, c->channels, c->sample_fmt,
                                     dst_samples_data[0], dst_samples_size, 0);
            encode_and_write_frame(frame);
            av_frame_unref(frame);
        } else if (s16_samples == NULL) {
            /* swr buffer empty and won't be more data */
            encode_and_write_frame(NULL);
            break;
        }

        nb_samples_total = swr_get_delay(swr_ctx, c->sample_rate);
        if (nb_samples_total < dst_nb_samples && s16_samples != NULL) break;

        /* swr buffered enough samples for another frame */
        /* reset swr [in] parameters to flush swr buffer */
        nb_samples = 0;
        s16_samples_ptr_arr = NULL;
    }
}

void close_audio(AVFormatContext *oc, AVStream *st)
{
    avcodec_close(st->codec);

    swr_free(&swr_ctx);
    av_free(dst_samples_data[0]);
    av_free(dst_samples_data);

    av_frame_free(&shared_frame);
}

void close()
{
    /* send EOF to encoder */
    encode_and_write(NULL, 0);

    /* Write the trailer, if any. The trailer must be written before you
     * close the CodecContexts open when you wrote the header; otherwise
     * av_write_trailer() may try to use memory that was freed on
     * av_codec_close(). */
    av_write_trailer(output_ctx);

    /* Close each codec. */
    close_audio(output_ctx, audio_st);

    /* Close the output file. */
    avio_close(output_ctx->pb);

    /* free the stream */
    avformat_free_context(output_ctx);
}

};

static float t, tincr, tincr2;

/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
 * 'nb_channels' channels. */
static void get_audio_frame(int16_t *samples, int frame_size, int nb_channels)
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

int main1(int argc, char **argv);
int main2(int argc, char **argv);

#define STREAM_DURATION 10
static const char filename[] = "out.mp3";

int main(int argc, char* argv[])
{
    //return main2(argc, argv);
    //return main1(argc, argv);
    av_register_all();
    av_log_set_level(AV_LOG_QUIET);

    do {
    Bue *bue;

    /* init signal generator */
    t     = 0;
    tincr = (float)(2 * M_PI * 110.0 / 48000);
    /* increment frequency by 110 Hz per second */
    tincr2 = (float)(2 * M_PI * 110.0 / 48000 / 48000);

    bue = new Bue();
    bue->open(filename);

    for (;;) {

        if (t/10000.0 >= STREAM_DURATION)
            break;
        enum { bufsz = 500 };
        int32_t samples[bufsz]; // aligned pairs
        get_audio_frame((int16_t*)samples, bufsz, 2);
        /* write interleaved audio and video frames */
        bue->encode_and_write((uint8_t*)samples, bufsz);
        //break;
    }

    bue->close();
    delete bue;
    } while(1);

    return 0;
}
