/*
 * Copyright (c) 2003 Fabrice Bellard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * libavformat API example.
 *
 * Output a media file in any supported libavformat format. The default
 * codecs are used.
 * @example muxing.c
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

#define SCALE_FLAGS SWS_BICUBIC
 #define OUTPUT_PCM	1
#define ALSA_GET_ENABLE 1
 #define MAX_AUDIO_FRAME_SIZE 192000



// a wrapper around a single output AVStream
typedef struct OutputStream {
    AVStream *st;
    AVCodecContext *enc;

    /* pts of the next frame that will be generated */
    int64_t next_pts;
    int samples_count;

    AVFrame *frame;
    AVFrame *tmp_frame;

    float t, tincr, tincr2;

    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
} OutputStream;


typedef struct OutputDev
{
	AVFormatContext *oc;
	int have_audio;
	int have_video;
	int encode_audio;
	int encode_video;
	OutputStream *video_st , *audio_st ;
}OutputDev;


typedef struct IntputDev {

	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFormatContext *a_ifmtCtx;
	int  audioindex;
	AVFrame *pAudioFrame;
	AVPacket *in_packet;
	struct SwrContext   *audio_convert_ctx;
	uint8_t *dst_buffer;
	int out_buffer_size;
}IntputDev;

typedef struct IntputViDev {

	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFormatContext *v_ifmtCtx;
	int  videoindex;
	struct SwsContext *img_convert_ctx;
	AVPacket *in_packet;
	AVFrame	*pFrame,*pFrameYUV;
}IntputViDev;


IntputDev alsa_input = { 0 };
IntputViDev video_input = { 0 };

OutputDev output_dev={ 0};

static void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    /* Write the compressed frame to the media file. */
    log_packet(fmt_ctx, pkt);
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

/* Add an output stream. */
static void add_stream(OutputStream *ost, AVFormatContext *oc,
                       AVCodec **codec,
                       enum AVCodecID codec_id)
{
    AVCodecContext *c;
    int i;

    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        exit(1);
    }

    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    ost->st->id = oc->nb_streams-1;
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        exit(1);
    }
    ost->enc = c;

    switch ((*codec)->type) {
     int default_sample_rate=48000;//44100
    case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt  = (*codec)->sample_fmts ?
            (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        c->bit_rate    = 64000;
        c->sample_rate = 48000;
        if ((*codec)->supported_samplerates) {
            c->sample_rate = (*codec)->supported_samplerates[0];
            for (i = 0; (*codec)->supported_samplerates[i]; i++) {
                if ((*codec)->supported_samplerates[i] == 48000)
                    c->sample_rate = 48000;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        c->channel_layout = AV_CH_LAYOUT_STEREO;
        if ((*codec)->channel_layouts) {
            c->channel_layout = (*codec)->channel_layouts[0];
            for (i = 0; (*codec)->channel_layouts[i]; i++) {
                if ((*codec)->channel_layouts[i] == AV_CH_LAYOUT_STEREO)
                    c->channel_layout = AV_CH_LAYOUT_STEREO;
            }
        }
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        ost->st->time_base = (AVRational){ 1, c->sample_rate };
        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;

        c->bit_rate = 400000;
        /* Resolution must be a multiple of two. */
        c->width    = 640;
        c->height   = 480;
        /* timebase: This is the fundamental unit of time (in seconds) in terms
         * of which frame timestamps are represented. For fixed-fps content,
         * timebase should be 1/framerate and timestamp increments should be
         * identical to 1. */
        ost->st->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        c->time_base       = ost->st->time_base;

        c->gop_size      = 12; /* emit one intra frame every twelve frames at most */
        c->pix_fmt       = STREAM_PIX_FMT;
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            /* just for testing, we also add B-frames */
            c->max_b_frames = 2;
        }
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            /* Needed to avoid using macroblocks in which some coeffs overflow.
             * This does not happen with normal video, it just happens here as
             * the motion of the chroma plane does not match the luma plane. */
            c->mb_decision = 2;
        }
    break;

    default:
        break;
    }

    /* Some formats want stream headers to be separate. */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

/**************************************************************/
/* audio output */

static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples)
{
    AVFrame *frame = av_frame_alloc();
    int ret;

    if (!frame) {
        fprintf(stderr, "Error allocating an audio frame\n");
        exit(1);
    }

    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;

    if (nb_samples) {
        ret = av_frame_get_buffer(frame, 0);
        if (ret < 0) {
            fprintf(stderr, "Error allocating an audio buffer\n");
            exit(1);
        }
    }

    return frame;
}



static void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    AVCodecContext *c;
    int nb_samples;
    int ret;
    AVDictionary *opt = NULL;

    c = ost->enc;

    /* open it */
    av_dict_copy(&opt, opt_arg, 0);
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open audio codec: %s\n", av_err2str(ret));
        exit(1);
    }

    /* init signal generator */
    ost->t     = 0;
    ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
    /* increment frequency by 110 Hz per second */
    ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

    if (c->codec->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE)
        nb_samples = 10000;
    else
        nb_samples = c->frame_size;

    ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                       c->sample_rate, nb_samples);
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
                                       c->sample_rate, nb_samples);

	printf( "c->channel_layout=%s channel=%d c->sample_fmt=%d  c->sample_rate=%d nb_samples=%d\n",
	  	 av_ts2str(c->channel_layout),c->channels,c->sample_rate,nb_samples,c->sample_fmt);

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }

    /* create resampler context */
        ost->swr_ctx = swr_alloc();
        if (!ost->swr_ctx) {
            fprintf(stderr, "Could not allocate resampler context\n");
            exit(1);
        }

        /* set options */
        av_opt_set_int       (ost->swr_ctx, "in_channel_count",   c->channels,       0);
        av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
        av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
        av_opt_set_int       (ost->swr_ctx, "out_channel_count",  c->channels,       0);
        av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
        av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);

        /* initialize the resampling context */
        if ((ret = swr_init(ost->swr_ctx)) < 0) {
            fprintf(stderr, "Failed to initialize the resampling context\n");
            exit(1);
        }
}

/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
 * 'nb_channels' channels. */
static AVFrame *get_audio_frame(OutputStream *ost)
{
    AVFrame *frame = ost->tmp_frame;
    int j, i, v;
    int16_t *q = (int16_t*)frame->data[0];

    /* check if we want to generate more frames */
    if (av_compare_ts(ost->next_pts, ost->enc->time_base,
                      STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
        return NULL;

    for (j = 0; j <frame->nb_samples; j++) {
        v = (int)(sin(ost->t) * 10000);
        for (i = 0; i < ost->enc->channels; i++)
            *q++ = v;
        ost->t     += ost->tincr;
        ost->tincr += ost->tincr2;
    }

    frame->pts = ost->next_pts;
    ost->next_pts  += frame->nb_samples;

    return frame;
}

/*
 * encode one audio frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
static int write_audio_frame(AVFormatContext *oc, OutputStream *ost)
{
    AVCodecContext *c;
    AVPacket pkt = { 0 }; // data and size must be 0;
    AVFrame *frame;
    int ret;
    int got_packet;
    int dst_nb_samples;

    av_init_packet(&pkt);
    c = ost->enc;

    frame = get_audio_frame(ost);

    if (frame) {
        /* convert samples from native format to destination codec format, using the resampler */
            /* compute destination number of samples */
            dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
                                            c->sample_rate, c->sample_rate, AV_ROUND_UP);
            av_assert0(dst_nb_samples == frame->nb_samples);

        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally;
         * make sure we do not overwrite it here
         */
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0)
            exit(1);

        /* convert to destination format */
        ret = swr_convert(ost->swr_ctx,
                          ost->frame->data, dst_nb_samples,
                          (const uint8_t **)frame->data, frame->nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            exit(1);
        }

        frame = ost->frame;

        frame->pts = av_rescale_q(ost->samples_count, (AVRational){1, c->sample_rate}, c->time_base);
        ost->samples_count += dst_nb_samples;
    }

    ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        fprintf(stderr, "Error encoding audio frame: %s\n", av_err2str(ret));
        exit(1);
    }

    if (got_packet) {
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
        if (ret < 0) {
            fprintf(stderr, "Error while writing audio frame: %s\n",
                    av_err2str(ret));
            exit(1);
        }
    }

    return (frame || got_packet) ? 0 : 1;
}

/**************************************************************/
static AVFrame *get_audio_frame1(OutputStream *ost,IntputDev* input,int *got_pcm)
{
    int j, i, v,ret,got_picture;
	AVFrame *ret_frame=NULL;

    AVFrame *frame = ost->tmp_frame;


	*got_pcm=1;
    /* check if we want to generate more frames */
    if (av_compare_ts(ost->next_pts, ost->enc->time_base,
                      STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
        return NULL;

  if(av_read_frame(input->a_ifmtCtx, input->in_packet)>=0){
		if(input->in_packet->stream_index==input->audioindex){
			ret = avcodec_decode_audio4(input->pCodecCtx, input->pAudioFrame , &got_picture, input->in_packet);

			*got_pcm=got_picture;

			if(ret < 0){
				printf("Decode Error.\n");
				av_free_packet(input->in_packet);
				return NULL;
			}
			if(got_picture){

				printf("src nb_samples %d dst nb-samples=%d out_buffer_size=%d\n",
					input->pAudioFrame->nb_samples,frame->nb_samples,input->out_buffer_size);

				  swr_convert(input->audio_convert_ctx, &input->dst_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)input->pAudioFrame->data, input->pAudioFrame->nb_samples);  

				frame->pts = ost->next_pts;
    				ost->next_pts  += frame->nb_samples;

				if(frame->nb_samples*4==input->out_buffer_size)//16bit stereo
				{
					//memcpy(frame->data,input->dst_buffer,input->out_buffer_size);
					memcpy(frame->data[0],input->dst_buffer,frame->nb_samples*4);
					ret_frame= frame;
				}
			}
		}
		av_free_packet(input->in_packet);
	}
    return frame;
}

static int write_audio_frame1(AVFormatContext *oc, OutputStream *ost,AVFrame *in_frame)
{
    AVCodecContext *c;
    AVPacket pkt = { 0 }; // data and size must be 0;
    int ret;
    int got_packet;
    int dst_nb_samples;

	//if(in_frame==NULL)
	//	return 1;


    av_init_packet(&pkt);

    AVFrame *frame=in_frame;

    c = ost->enc;

    if (frame) {
        /* convert samples from native format to destination codec format, using the resampler */
            /* compute destination number of samples */
            dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
                                            c->sample_rate, c->sample_rate, AV_ROUND_UP);
            av_assert0(dst_nb_samples == frame->nb_samples);

        /* when we pass a frame to the encoder, it may keep a reference to it
         * internally;
         * make sure we do not overwrite it here
         */
        ret = av_frame_make_writable(ost->frame);
        if (ret < 0)
            exit(1);

        /* convert to destination format */
        ret = swr_convert(ost->swr_ctx,
                          ost->frame->data, dst_nb_samples,
                          (const uint8_t **)frame->data, frame->nb_samples);
        if (ret < 0) {
            fprintf(stderr, "Error while converting\n");
            exit(1);
        }
        frame = ost->frame;

        frame->pts = av_rescale_q(ost->samples_count, (AVRational){1, c->sample_rate}, c->time_base);
        ost->samples_count += dst_nb_samples;
    }

    ret = avcodec_encode_audio2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        fprintf(stderr, "Error encoding audio frame: %s\n", av_err2str(ret));
        exit(1);
    }

    if (got_packet) {
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
        if (ret < 0) {
            fprintf(stderr, "Error while writing audio frame: %s\n",
                    av_err2str(ret));
            exit(1);
        }
    }

    return (frame || got_packet) ? 0 : 1;
}



static void close_stream(AVFormatContext *oc, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}


/**************************************************************/
/* video output */

static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;
    int ret;

    picture = av_frame_alloc();
    if (!picture)
        return NULL;

    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    ret = av_frame_get_buffer(picture, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate frame data.\n");
        exit(1);
    }

    return picture;
}

static void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    int ret;
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = NULL;

    av_dict_copy(&opt, opt_arg, 0);

    /* open the codec */
    ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
        exit(1);
    }

    /* allocate and init a re-usable frame */
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    if (!ost->frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }


        printf("ost->frame alloc success fmt=%d w=%d h=%d\n",c->pix_fmt,c->width, c->height);


    /* If the output format is not YUV420P, then a temporary YUV420P
     * picture is needed too. It is then converted to the required
     * output format. */
    ost->tmp_frame = NULL;
    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        ost->tmp_frame = alloc_picture(AV_PIX_FMT_YUV420P, c->width, c->height);
        if (!ost->tmp_frame) {
            fprintf(stderr, "Could not allocate temporary picture\n");
            exit(1);
        }
    }

    /* copy the stream parameters to the muxer */
    ret = avcodec_parameters_from_context(ost->st->codecpar, c);
    if (ret < 0) {
        fprintf(stderr, "Could not copy the stream parameters\n");
        exit(1);
    }
}

/* Prepare a dummy image. */
static void fill_yuv_image(AVFrame *pict, int frame_index,
                           int width, int height)
{
    int x, y, i;

    i = frame_index;

    /* Y */
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            pict->data[0][y * pict->linesize[0] + x] = x + y + i * 3;

    /* Cb and Cr */
    for (y = 0; y < height / 2; y++) {
        for (x = 0; x < width / 2; x++) {
            pict->data[1][y * pict->linesize[1] + x] = 128 + y + i * 2;
            pict->data[2][y * pict->linesize[2] + x] = 64 + x + i * 5;
        }
    }
}

static AVFrame *get_video_frame(OutputStream *ost)
{
    AVCodecContext *c = ost->enc;

    /* check if we want to generate more frames */
    if (av_compare_ts(ost->next_pts, c->time_base,
                      STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
        return NULL;

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally; make sure we do not overwrite it here */
    if (av_frame_make_writable(ost->frame) < 0)
        exit(1);

    if (c->pix_fmt != AV_PIX_FMT_YUV420P) {
        /* as we only generate a YUV420P picture, we must convert it
         * to the codec pixel format if needed */
        if (!ost->sws_ctx) {
            ost->sws_ctx = sws_getContext(c->width, c->height,
                                          AV_PIX_FMT_YUV420P,
                                          c->width, c->height,
                                          c->pix_fmt,
                                          SCALE_FLAGS, NULL, NULL, NULL);
            if (!ost->sws_ctx) {
                fprintf(stderr,
                        "Could not initialize the conversion context\n");
                exit(1);
            }
        }
        fill_yuv_image(ost->tmp_frame, ost->next_pts, c->width, c->height);
        sws_scale(ost->sws_ctx, (const uint8_t * const *) ost->tmp_frame->data,
                  ost->tmp_frame->linesize, 0, c->height, ost->frame->data,
                  ost->frame->linesize);
    } else {
        fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
    }

    ost->frame->pts = ost->next_pts++;

    return ost->frame;
}

/*
 * encode one video frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
static int write_video_frame(AVFormatContext *oc, OutputStream *ost)
{
    int ret;
    AVCodecContext *c;
    AVFrame *frame;
    int got_packet = 0;
    AVPacket pkt = { 0 };

    c = ost->enc;

    frame = get_video_frame(ost);

    av_init_packet(&pkt);

    /* encode the image */
    ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        fprintf(stderr, "Error encoding video frame: %s\n", av_err2str(ret));
        exit(1);
    }

    if (got_packet) {
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
    } else {
        ret = 0;
    }

    if (ret < 0) {
        fprintf(stderr, "Error while writing video frame: %s\n", av_err2str(ret));
        exit(1);
    }

    return (frame || got_packet) ? 0 : 1;
}



/*
 * encode one video frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
static int write_video_frame1(AVFormatContext *oc, OutputStream *ost,AVFrame *frame)
{
    int ret;
    AVCodecContext *c;
    int got_packet = 0;
    AVPacket pkt = { 0 };

	//if(frame==NULL)
	//	return 1;


    c = ost->enc;


    av_init_packet(&pkt);


    /* encode the image */
    ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        fprintf(stderr, "Error encoding video frame: %s\n", av_err2str(ret));
        exit(1);
    }


	printf("--------------video- pkt.pts=%s\n",av_ts2str(pkt.pts));

		printf("----st.num=%d st.den=%d codec.num=%d codec.den=%d---------\n",ost->st->time_base.num,ost->st->time_base.den,
			c->time_base.num,c->time_base.den);


    if (got_packet) {
        ret = write_frame(oc, &c->time_base, ost->st, &pkt);
    } else {
        ret = 0;
    }

    if (ret < 0) {
        fprintf(stderr, "Error while writing video frame: %s\n", av_err2str(ret));
        exit(1);
    }

    return (frame || got_packet) ? 0 : 1;
}


static AVFrame *get_video_frame1(OutputStream *ost,IntputViDev* input,int *got_pic)
{

	int ret, got_picture;
    	AVCodecContext *c = ost->enc;
	AVFrame * ret_frame=NULL;
	if (av_compare_ts(ost->next_pts, c->time_base,
                      STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
        return NULL;

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally; make sure we do not overwrite it here */
    if (av_frame_make_writable(ost->frame) < 0)
        exit(1);
	

	if(av_read_frame(input->v_ifmtCtx, input->in_packet)>=0){
		if(input->in_packet->stream_index==input->videoindex){
			ret = avcodec_decode_video2(input->pCodecCtx, input->pFrame, &got_picture, input->in_packet);
			*got_pic=got_picture;

			if(ret < 0){
				printf("Decode Error.\n");
				av_free_packet(input->in_packet);
				return NULL;
			}
			if(got_picture){
				//sws_scale(input->img_convert_ctx, (const unsigned char* const*)input->pFrame->data, input->pFrame->linesize, 0, input->pCodecCtx->height, ost->frame->data, ost->frame->linesize);
				sws_scale(input->img_convert_ctx, (const unsigned char* const*)input->pFrame->data, input->pFrame->linesize, 0, input->pCodecCtx->height, ost->frame->data,  ost->frame->linesize);
				ost->frame->pts =ost->next_pts++;
				ret_frame= ost->frame;
				
			}
		}
		av_free_packet(input->in_packet);
	}
	return ret_frame;
}


/**************************************************************/
/* media file output */

static int flush_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index)  
{  
    int ret;  
    int got_frame;  
    AVPacket enc_pkt;  
    if (!(fmt_ctx->streams[stream_index]->codec->codec->capabilities &  
        AV_CODEC_CAP_DELAY))  
        return 0;  
    while (1) {  
        printf("Flushing stream #%u encoder\n", stream_index);  
        //ret = encode_write_frame(NULL, stream_index, &got_frame);  
        enc_pkt.data = NULL;  
        enc_pkt.size = 0;  
        av_init_packet(&enc_pkt);  
        ret = avcodec_encode_audio2 (fmt_ctx->streams[stream_index]->codec, &enc_pkt,  
            NULL, &got_frame);  
        av_frame_free(NULL);  
        if (ret < 0)  
            break;  
        if (!got_frame){  
            ret=0;  
            break;  
        }  
        printf("Succeed to encode 1 frame! \n");  
        /* mux encoded frame */  
        ret = av_interleaved_write_frame(fmt_ctx, &enc_pkt);  
        if (ret < 0)  
            break;  
    }  
    return ret;  
}  

int open_audio_capture()
{

		printf("open_audio_capture\n");

//********add alsa read***********//
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
       AVFormatContext *a_ifmtCtx;
	int i,ret;
//Register Device
	avdevice_register_all();

	a_ifmtCtx = avformat_alloc_context();


	 //Linux
	AVInputFormat *ifmt=av_find_input_format("alsa");
	if(avformat_open_input(&a_ifmtCtx,"default",ifmt,NULL)!=0){
		printf("Couldn't open input stream.default\n");
		return -1;
	}
 
 
	if(avformat_find_stream_info(a_ifmtCtx,NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	int audioindex=-1;
	for(i=0; i<a_ifmtCtx->nb_streams; i++) 
	if(a_ifmtCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
	{
		audioindex=i;
		break;
	}
	if(audioindex==-1)
	{
		printf("Couldn't find a video stream.\n");
		return -1;
	}
		
	pCodecCtx=a_ifmtCtx->streams[audioindex]->codec;
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	AVPacket *in_packet=(AVPacket *)av_malloc(sizeof(AVPacket));

	AVFrame *pAudioFrame=av_frame_alloc();
	if(NULL==pAudioFrame)
	{
		printf("could not alloc pAudioFrame\n");
		return -1;
	}

	//audio output paramter //resample 
	uint64_t out_channel_layout = AV_CH_LAYOUT_STEREO;
	int out_sample_fmt = AV_SAMPLE_FMT_S16;
	int out_nb_samples =1024; //pCodecCtx->frame_size;
	int out_sample_rate = 48000;
	int out_nb_channels = av_get_channel_layout_nb_channels(out_channel_layout);
	int out_buffer_size = av_samples_get_buffer_size(NULL, out_nb_channels, out_nb_samples, out_sample_fmt, 1);  
	uint8_t *dst_buffer=NULL;  
	dst_buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE); 
	int64_t in_channel_layout = av_get_default_channel_layout(pCodecCtx->channels);  


	printf("audio sample_fmt=%d size=%d channel=%d  sample_rate=%d in_channel_layout=%s\n",
		pCodecCtx->sample_fmt, pCodecCtx->frame_size,
		pCodecCtx->channels,pCodecCtx->sample_rate,av_ts2str(in_channel_layout));

	struct SwrContext   *audio_convert_ctx = NULL;  
	audio_convert_ctx = swr_alloc();  
	if (audio_convert_ctx == NULL)  
	{  
	    printf("Could not allocate SwrContext\n");  
	    return -1;  
	}  

	  /* set options */
        av_opt_set_int       (audio_convert_ctx, "in_channel_count",   pCodecCtx->channels,       0);
        av_opt_set_int       (audio_convert_ctx, "in_sample_rate",     pCodecCtx->sample_rate,    0);
        av_opt_set_sample_fmt(audio_convert_ctx, "in_sample_fmt",      pCodecCtx->sample_fmt, 0);
        av_opt_set_int       (audio_convert_ctx, "out_channel_count",  out_nb_channels,       0);
        av_opt_set_int       (audio_convert_ctx, "out_sample_rate",   out_sample_rate,    0);
        av_opt_set_sample_fmt(audio_convert_ctx, "out_sample_fmt",     out_sample_fmt,     0);

        /* initialize the resampling context */
        if ((ret = swr_init(audio_convert_ctx)) < 0) {
            fprintf(stderr, "Failed to initialize the resampling context\n");
            exit(1);
        }


	alsa_input.in_packet=in_packet;
	alsa_input.pCodecCtx=pCodecCtx;
	alsa_input.pCodec=pCodec;
       alsa_input.a_ifmtCtx=a_ifmtCtx;
   	alsa_input.audioindex=audioindex;
	alsa_input.pAudioFrame=pAudioFrame;
	alsa_input.audio_convert_ctx=audio_convert_ctx;
	alsa_input.dst_buffer=dst_buffer;
	alsa_input.out_buffer_size=out_buffer_size;
 
//******************************//
}


int open_video_capture()
{
	int i,ret;
	printf("open_video_capture\n");

//********add camera read***********//
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
       AVFormatContext *v_ifmtCtx;

//Register Device
	avdevice_register_all();

	v_ifmtCtx = avformat_alloc_context();


	 //Linux
	AVInputFormat *ifmt=av_find_input_format("video4linux2");
	if(avformat_open_input(&v_ifmtCtx,"/dev/video0",ifmt,NULL)!=0){
		printf("Couldn't open input stream./dev/video0\n");
		return -1;
	}
 
 
	if(avformat_find_stream_info(v_ifmtCtx,NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}

	int videoindex=-1;
	for(i=0; i<v_ifmtCtx->nb_streams; i++) 
	if(v_ifmtCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
	{
		videoindex=i;
		break;
	}
	if(videoindex==-1)
	{
		printf("Couldn't find a video stream.\n");
		return -1;
	}
		
	pCodecCtx=v_ifmtCtx->streams[videoindex]->codec;
	pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
	if(pCodec==NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	if(avcodec_open2(pCodecCtx, pCodec,NULL)<0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	AVFrame	*pFrame,*pFrameYUV;
	pFrame=av_frame_alloc();
	pFrameYUV=av_frame_alloc();
	unsigned char *out_buffer=(unsigned char *)av_malloc(avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
	avpicture_fill((AVPicture *)pFrameYUV, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height);

	printf("camera width=%d height=%d \n",pCodecCtx->width, pCodecCtx->height);


	struct SwsContext *img_convert_ctx;
	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL); 
	AVPacket *in_packet=(AVPacket *)av_malloc(sizeof(AVPacket));


	video_input.img_convert_ctx=img_convert_ctx;
	video_input.in_packet=in_packet;

	video_input.pCodecCtx=pCodecCtx;
	video_input.pCodec=pCodec;
       video_input.v_ifmtCtx=v_ifmtCtx;
   	video_input.videoindex=videoindex;
	video_input.pFrame=pFrame;
	video_input.pFrameYUV=pFrameYUV;

//******************************//
}



int open_output(    const char *filename,AVDictionary *opt)
{

	printf("open_output\n");
	static	OutputStream video_st = { 0 }, audio_st = { 0 };

    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVCodec *audio_codec, *video_codec;
    int ret;
    int have_video = 0, have_audio = 0;
    int encode_video = 0, encode_audio = 0;

	
	   /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, NULL, filename);
    if (!oc) {
        printf("Could not deduce output format from file extension: using MPEG.\n");
        avformat_alloc_output_context2(&oc, NULL, "mpeg", filename);
    }
    if (!oc)
        return 1;

     fmt = oc->oformat;

    /* Add the audio and video streams using the default format codecs
     * and initialize the codecs. */
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        add_stream(&video_st, oc, &video_codec, fmt->video_codec);
        have_video = 1;
        encode_video = 1;
    }
    if (fmt->audio_codec != AV_CODEC_ID_NONE) {
        add_stream(&audio_st, oc, &audio_codec, AV_CODEC_ID_AAC);//fmt->audio_codec);
        have_audio = 1;
        encode_audio = 1;
    }

    /* Now that all the parameters are set, we can open the audio and
     * video codecs and allocate the necessary encode buffers. */
    if (have_video)
        open_video(oc, video_codec, &video_st, opt);

    if (have_audio)
        open_audio(oc, audio_codec, &audio_st, opt);

    av_dump_format(oc, 0, filename, 1);

    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open '%s': %s\n", filename,
                    av_err2str(ret));
            return 1;
        }
    }

    /* Write the stream header, if any. */
    ret = avformat_write_header(oc, &opt);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file: %s\n",
                av_err2str(ret));
        return 1;
    }


	output_dev.encode_audio=encode_audio;
	output_dev.encode_video=encode_video;
	output_dev.oc=oc;
	output_dev.have_audio=have_audio;
	output_dev.have_video=have_video;
	output_dev.video_st=&video_st;
	output_dev.audio_st=&audio_st;

}

int main(int argc, char **argv)
{
	const char *filename;
	AVDictionary *opt = NULL;
	int i;

    if (argc < 2) {
        printf("usage: %s output_file\n"
               "API example program to output a media file with libavformat.\n"
               "This program generates a synthetic audio and video stream, encodes and\n"
               "muxes them into a file named output_file.\n"
               "The output format is automatically guessed according to the file extension.\n"
               "Raw images can also be output by using '%%d' in the filename.\n"
               "\n", argv[0]);
        return 1;
    }

    filename = argv[1];
    for (i = 2; i+1 < argc; i+=2) {
        if (!strcmp(argv[i], "-flags") || !strcmp(argv[i], "-fflags"))
            av_dict_set(&opt, argv[i]+1, argv[i+1], 0);
    }

	open_audio_capture();
	open_video_capture();

	open_output(filename,opt);

 	printf("init finish\n");

	int got_pcm,got_pic;

    while (output_dev.encode_video || output_dev.encode_audio) { //){//
        /* select the stream to encode */

		//output_dev.encode_video=0;
		if (output_dev.encode_video &&
		(!output_dev.encode_audio || av_compare_ts(output_dev.video_st->next_pts, output_dev.video_st->enc->time_base,
		        output_dev.audio_st->next_pts, output_dev.audio_st->enc->time_base) <= 0)) 
		{
		        
			AVFrame *vframe=get_video_frame1(output_dev.video_st,&video_input,&got_pic);
			if(!got_pic)
			{
				usleep(10000);
				continue;

			}
			output_dev.encode_video = !write_video_frame1(output_dev.oc, output_dev.video_st,vframe);
		}

	 	else
	 	{
			AVFrame *aframe=get_audio_frame1(output_dev.audio_st,&alsa_input,&got_pcm);
			if(!got_pcm)
			{
				printf("get_audio_frame1 Error.\n");
				usleep(10000);
				continue;
			}

			output_dev.encode_audio = !write_audio_frame1(output_dev.oc, output_dev.audio_st,aframe);

	}

		


    }
    av_write_trailer(output_dev.oc);

	usleep(100000);
//audio 
 	printf("free audio\n");

	swr_free(&alsa_input.audio_convert_ctx); 
	avcodec_close(alsa_input.pCodecCtx);	
	av_free(alsa_input.pAudioFrame);
	av_free(alsa_input.dst_buffer); 
	avformat_close_input(&alsa_input.a_ifmtCtx);


//vidoe 

	usleep(100000);

 	printf("free video\n");

	sws_freeContext(video_input.img_convert_ctx);
	avcodec_close(video_input.pCodecCtx);
	av_free(video_input.pFrameYUV);
	av_free(video_input.pFrame);	
	avformat_close_input(&video_input.v_ifmtCtx);

//output
	usleep(100000);

 	printf("free output\n");
/* Close each codec. */
 	printf("free output video_st\n");

	if (output_dev.have_video)
		close_stream(output_dev.oc, output_dev.video_st);
 	printf("free output audio_st\n");

	if (output_dev.have_audio)
		close_stream(output_dev.oc, output_dev.audio_st);


 	printf("free output avio_closep\n");
	if (!(output_dev.oc->oformat->flags & AVFMT_NOFILE))
/* Close the output file. */
		avio_closep(&output_dev.oc->pb);

 	printf("free output oc\n");

    /* free the stream */
    avformat_free_context(output_dev.oc);
 	printf("free finish\n");

    return 0;
}


