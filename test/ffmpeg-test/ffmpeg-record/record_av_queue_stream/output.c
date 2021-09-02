#include "common.h"


OutputDev output_dev={ 0};

  void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt)
{
    AVRational *time_base = &fmt_ctx->streams[pkt->stream_index]->time_base;

    printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
           av_ts2str(pkt->pts), av_ts2timestr(pkt->pts, time_base),
           av_ts2str(pkt->dts), av_ts2timestr(pkt->dts, time_base),
           av_ts2str(pkt->duration), av_ts2timestr(pkt->duration, time_base),
           pkt->stream_index);
}

  int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;

    /* Write the compressed frame to the media file. */
    //log_packet(fmt_ctx, pkt);
    return av_interleaved_write_frame(fmt_ctx, pkt);
}

/* Add an output stream. */
  void add_stream(OutputStream *ost, AVFormatContext *oc,
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

        c->bit_rate = 200000;
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

  void close_stream(AVFormatContext *oc, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}
/* media file output */

  int flush_encoder(AVFormatContext *fmt_ctx,unsigned int stream_index)  
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

int av_encode_frame(AVCodecContext *enc_ctx,AVPacket *packet,AVFrame *frame, int *got_packet_ptr)
{
	*got_packet_ptr=0;
	int ret=-1;
	ret=avcodec_send_frame(enc_ctx,frame);
	if(ret<0)
	{
        	printf("avcodec_send_frame ERROR \n");  
		//return ret;
	}

	ret=avcodec_receive_packet(enc_ctx,packet);
	 if(ret==AVERROR(EAGAIN))
	{
		*got_packet_ptr=0;
		packet=NULL;
        	printf("avcodec_receive_packet EAGAIN \n");  
	}
	else if(ret<0)
	{
		*got_packet_ptr=0;
        	printf("avcodec_receive_packet ERROR \n");  
	}
	else{
		*got_packet_ptr=1;
	}
	
	return ret;
		
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


//check ouput file 
    char push_stream = 0;  
    char *ofmt_name = NULL;  
    if (strstr(filename, "rtmp://") != NULL)  
    {  
        push_stream = 1;  
        ofmt_name = "flv";  
    }  
    else if (strstr(filename, "udp://") != NULL)  
    {  
        push_stream = 1;  
        ofmt_name = "mpegts";  
    }  
	else if (strstr(filename, "rtp://") != NULL)  
    {  
        push_stream = 1;  
        ofmt_name = "rtp_mpegts";  
    }  
    else  
    {  
        push_stream = 0;  
        ofmt_name = NULL;  
    }  
	
	   /* allocate the output media context */
    avformat_alloc_output_context2(&oc, NULL, ofmt_name, filename);
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
	output_dev.push_stream=push_stream;

}

 int capture_start(PacketQueue *queue, int (*fn)(void *), const char *thread_name, void* arg)
{
	packet_queue_init(queue);
	packet_queue_start(queue);
	 int decoder_tid= SDL_CreateThread(fn, arg);
	if (!decoder_tid) {
		av_log(NULL, AV_LOG_ERROR, "SDL_CreateThread(): %s\n", SDL_GetError());
		return AVERROR(ENOMEM);
	}
    return 0;

}

