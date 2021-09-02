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

 #ifndef RECORD_AV_QUEUE_COMMON_H
#define RECORD_AV_QUEUE_COMMON_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <SDL_thread.h>


#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#define STREAM_DURATION  50.0
#define STREAM_FRAME_RATE 20 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

#define SCALE_FLAGS SWS_BICUBIC
 #define OUTPUT_PCM	1
#define ALSA_GET_ENABLE 1
 #define MAX_AUDIO_FRAME_SIZE 192000




typedef struct MyAVPacketList {
    AVPacket pkt;
    int64_t frame_pts;//pts from frame
    struct MyAVPacketList *next;
} MyAVPacketList;
 
typedef struct PacketQueue {
    MyAVPacketList *first_pkt, *last_pkt;
    int nb_packets;
    int size;
    int64_t duration;
    int abort_request;
    SDL_mutex *mutex;
    SDL_cond *cond;
} PacketQueue;
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
	PacketQueue videoq;
	PacketQueue audioq;
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
	char bCap;

}IntputDev;

typedef struct IntputViDev {

	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	AVFormatContext *v_ifmtCtx;
	int  videoindex;
	struct SwsContext *img_convert_ctx;
	AVPacket *in_packet;
	AVFrame	*pFrame,*pFrameYUV;
	char bCap;

}IntputViDev;

 

 void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt);
 

 int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);
 

/* Add an output stream. */
 void add_stream(OutputStream *ost, AVFormatContext *oc,
                       AVCodec **codec,
                       enum AVCodecID codec_id);
 

/**************************************************************/
/* audio output */

  AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples);
 



  void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
 

/* Prepare a 16 bit dummy audio frame of 'frame_size' samples and
 * 'nb_channels' channels. */
  AVFrame *get_audio_frame(OutputStream *ost);
 

/*
 * encode one audio frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
  int write_audio_frame(AVFormatContext *oc, OutputStream *ost);
 

/**************************************************************/
  AVFrame *get_audio_frame1(OutputStream *ost,IntputDev* input,int *got_pcm);
 

  int write_audio_frame1(AVFormatContext *oc, OutputStream *ost,AVFrame *in_frame);
 
 AVFrame *get_audio_pkt2Frame(OutputStream *ost,IntputDev* input,AVPacket *pkt,int *got_pcm, int64_t pts);
 AVPacket *get_audio_pkt(OutputStream *ost,IntputDev* input);
int audioThreadProc(void *arg);


  void close_stream(AVFormatContext *oc, OutputStream *ost);
 

 AVFrame *get_video_pkt2Frame(OutputStream *ost,IntputViDev* input,AVPacket *pkt,int *got_pic, int64_t pts);

 AVPacket *get_video_pkt(OutputStream *ost,IntputViDev* input);


/**************************************************************/
/* video output */

  AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
 

  void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
 

/* Prepare a dummy image. */
  void fill_yuv_image(AVFrame *pict, int frame_index,  int width, int height);
 

  AVFrame *get_video_frame(OutputStream *ost);
 

/*
 * encode one video frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
  int write_video_frame(AVFormatContext *oc, OutputStream *ost);
 



/*
 * encode one video frame and send it to the muxer
 * return 1 when encoding is finished, 0 otherwise
 */
  int write_video_frame1(AVFormatContext *oc, OutputStream *ost,AVFrame *frame);
 


  AVFrame *get_video_frame1(OutputStream *ost,IntputViDev* input,int *got_pic);
 


/**************************************************************/
/* media file output */

  int flush_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index)  ;
   

int open_audio_capture();
 


int open_video_capture();
 



int open_output(    const char *filename,AVDictionary *opt);
 
int videoThreadProc(void *arg);


int capture_start(PacketQueue *queue, int (*fn)(void *), const char *thread_name, void* arg);


int main(int argc, char **argv);


#endif //RECORD_AV_QUEUE_COMMON_H
