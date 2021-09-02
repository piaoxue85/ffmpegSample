

#include "common.h"

extern OutputDev output_dev;


IntputDev alsa_input = { 0 };

/**************************************************************/
/* audio output */

  AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
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



  void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
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
  AVFrame *get_audio_frame(OutputStream *ost)
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
  int write_audio_frame(AVFormatContext *oc, OutputStream *ost)
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
  AVFrame *get_audio_frame1(OutputStream *ost,IntputDev* input,int *got_pcm)
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

				//printf("src nb_samples %d dst nb-samples=%d out_buffer_size=%d\n",
				//	input->pAudioFrame->nb_samples,frame->nb_samples,input->out_buffer_size);

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


 AVPacket *get_audio_pkt(OutputStream *ost,IntputDev* input)
{
    int j, i, v,ret,got_picture;
	AVFrame *ret_frame=NULL;

    AVFrame *frame = ost->tmp_frame;

	AVPacket * ret_pkt=NULL;

    /* check if we want to generate more frames */
    if (av_compare_ts(ost->next_pts, ost->enc->time_base,
                      STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
        return NULL;

  	if(av_read_frame(input->a_ifmtCtx, input->in_packet)>=0){
		if(input->in_packet->stream_index==input->audioindex){

			ret_pkt=input->in_packet;
			ost->next_pts  += frame->nb_samples;
		}
		//av_free_packet(input->in_packet);
	}
    return ret_pkt;
}

 AVFrame *get_audio_pkt2Frame(OutputStream *ost,IntputDev* input,AVPacket *pkt,int *got_pcm, int64_t pts)
{
	AVFrame * ret_frame=NULL;
	int ret,got_picture=0;
    	AVFrame *frame = ost->tmp_frame;

	if(pkt!=NULL)
	{
		ret = avcodec_decode_audio4(input->pCodecCtx, input->pAudioFrame , &got_picture, pkt);

		*got_pcm=got_picture;

		if(ret < 0){
			printf("Decode Error.\n");
			//av_free_packet(input->in_packet);
			return NULL;
		}
		if(got_picture){

			//printf("src nb_samples %d dst nb-samples=%d out_buffer_size=%d\n",
			//	input->pAudioFrame->nb_samples,frame->nb_samples,input->out_buffer_size);

			  swr_convert(input->audio_convert_ctx, &input->dst_buffer, MAX_AUDIO_FRAME_SIZE, (const uint8_t **)input->pAudioFrame->data, input->pAudioFrame->nb_samples);  

			frame->pts =pts;
				//ost->next_pts  += frame->nb_samples;

			if(frame->nb_samples*4==input->out_buffer_size)//16bit stereo
			{
				//memcpy(frame->data,input->dst_buffer,input->out_buffer_size);
				memcpy(frame->data[0],input->dst_buffer,frame->nb_samples*4);
				ret_frame= frame;
			}
		}
	}
	return ret_frame;
}


  int write_audio_frame1(AVFormatContext *oc, OutputStream *ost,AVFrame *in_frame)
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

	printf("==========audio- =========\n");

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
	alsa_input.bCap=1;
 
//******************************//
}


int audioThreadProc(void *arg)
{
	int got_pic;
	while(alsa_input.bCap)
	{

		//printf("audioThreadProc running\n");

		AVPacket *pkt=get_audio_pkt(output_dev.audio_st,&alsa_input);
		if(pkt==NULL)
		{
			alsa_input.bCap =0;

		}
		else
		{
			packet_queue_put(&output_dev.audioq,pkt,output_dev.audio_st->next_pts);
		}


	}

	printf("videoThreadProc exit\n");
	usleep(1000000);
	return 0;
	
}

