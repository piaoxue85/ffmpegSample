
#include "common.h"

extern OutputDev output_dev;


IntputViDev video_input = { 0 };

/**************************************************************/
/* video output */

  AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
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

  void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
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
  void fill_yuv_image(AVFrame *pict, int frame_index,
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

  AVFrame *get_video_frame(OutputStream *ost)
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
  int write_video_frame(AVFormatContext *oc, OutputStream *ost)
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
  int write_video_frame1(AVFormatContext *oc, OutputStream *ost,AVFrame *frame)
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
   /* ret = avcodec_encode_video2(c, &pkt, frame, &got_packet);
    if (ret < 0) {
        fprintf(stderr, "Error encoding video frame: %s\n", av_err2str(ret));

        exit(1);
    }*/

	ret=av_encode_frame(c, &pkt, frame,&got_packet);

	if(ret < 0){
		printf("video av_encode_frame Error.\n");
		//return NULL;
	}
	if (debug_ts) {	
		printf("--------------video- pkt.pts=%s\n",av_ts2str(pkt.pts));
	}
		//printf("----st.num=%d st.den=%d codec.num=%d codec.den=%d---------\n",ost->st->time_base.num,ost->st->time_base.den,
		//	c->time_base.num,c->time_base.den);
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


  AVFrame *get_video_frame1(OutputStream *ost,IntputViDev* input,int *got_pic)
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


  AVPacket *get_video_pkt(OutputStream *ost,IntputViDev* input)
{

	int ret, got_picture;
    	AVCodecContext *c = ost->enc;
	AVPacket * ret_pkt=NULL;

	AVStream *in_stream;
	in_stream=input->v_ifmtCtx->streams[input->videoindex];

	int64_t pkt_dts;
	

    /* when we pass a frame to the encoder, it may keep a reference to it
     * internally; make sure we do not overwrite it here */
    if (av_frame_make_writable(ost->frame) < 0)
        exit(1);
	

	if(av_read_frame(input->v_ifmtCtx, input->in_packet)>=0){
		if(input->in_packet->stream_index==input->videoindex){

			if(input->ts_offset_flg==0)
			{
				input->ts_offset_flg=1;
				input->ts_offset=-input->in_packet->pts;
			}
			if (debug_ts) {
				av_log(NULL, AV_LOG_INFO,"000 video pkt_pts:%s pkt_pts_time:%s pkt_dts:%s pkt_dts_time:%s off:%s off_time:%s\n",
				av_ts2str(input->in_packet->pts), av_ts2timestr(input->in_packet->pts,&input->v_ifmtCtx->streams[input->videoindex]->time_base),
				av_ts2str(input->in_packet->dts), av_ts2timestr(input->in_packet->dts, &input->v_ifmtCtx->streams[input->videoindex]->time_base),
				av_ts2str(input->ts_offset),
				av_ts2timestr(input->ts_offset, &AV_TIME_BASE_Q));
    			}

			if (input->in_packet->dts != AV_NOPTS_VALUE)
				input->in_packet->dts += av_rescale_q(input->ts_offset, AV_TIME_BASE_Q, input->v_ifmtCtx->streams[input->videoindex]->time_base);
			if (input->in_packet->pts != AV_NOPTS_VALUE)
				input->in_packet->pts += av_rescale_q(input->ts_offset, AV_TIME_BASE_Q, input->v_ifmtCtx->streams[input->videoindex]->time_base);


			
	
			if (debug_ts) {
					av_log(NULL, AV_LOG_INFO,"111 video pkt_pts:%s pkt_pts_time:%s pkt_dts:%s pkt_dts_time:%s off:%s off_time:%s\n",
					av_ts2str(input->in_packet->pts), av_ts2timestr(input->in_packet->pts,&input->v_ifmtCtx->streams[input->videoindex]->time_base),
					av_ts2str(input->in_packet->dts), av_ts2timestr(input->in_packet->dts, &input->v_ifmtCtx->streams[input->videoindex]->time_base),
					av_ts2str(input->ts_offset),
					av_ts2timestr(input->ts_offset, &AV_TIME_BASE_Q));
	    			}

			pkt_dts = av_rescale_q_rnd(input->in_packet->dts, in_stream->time_base, AV_TIME_BASE_Q, AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX);
			if ( pkt_dts != AV_NOPTS_VALUE && input->last_ts != AV_NOPTS_VALUE) {
				int64_t delta   = pkt_dts - input->last_ts;
				if (debug_ts) {
					av_log(NULL, AV_LOG_INFO,"111 video delta:%s\n",
					av_ts2str(delta));
				}

				if (delta < -1LL*dts_delta_threshold*AV_TIME_BASE ||
								delta >  1LL*dts_delta_threshold*AV_TIME_BASE){
					input->ts_offset -= delta;
					av_log(NULL, AV_LOG_DEBUG,
					"Inter stream timestamp discontinuity %"PRId64", new offset= %"PRId64"\n",
					delta, input->ts_offset);
					input->in_packet->dts -= av_rescale_q(delta, AV_TIME_BASE_Q, in_stream->time_base);
					if (input->in_packet->pts != AV_NOPTS_VALUE)
					input->in_packet->pts -= av_rescale_q(delta, AV_TIME_BASE_Q, in_stream->time_base);
				}
			}
			

			if(input->in_packet->pts!=0)
			{
				if (av_compare_ts(input->in_packet->pts, input->v_ifmtCtx->streams[input->videoindex]->time_base,
                      	STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
				{
					av_free_packet(input->in_packet);
		       		 return NULL;
				}
			}

			if (input->in_packet->dts != AV_NOPTS_VALUE)
        			input->last_ts = av_rescale_q(input->in_packet->dts, in_stream->time_base, AV_TIME_BASE_Q);

			
			ret_pkt=input->in_packet;
			ost->next_pts=input->in_packet->pts;
		}
		//av_free_packet(input->in_packet);
	}
	return ret_pkt;
}


 AVFrame *get_video_pkt2Frame(OutputStream *ost,IntputViDev* input,AVPacket *pkt,int *got_pic, int64_t pts)
{
	AVFrame * ret_frame=NULL;
	int ret,got_picture=0;
	if(pkt!=NULL)
	{
		ret = avcodec_decode_video2(input->pCodecCtx, input->pFrame, &got_picture, pkt);
		*got_pic=got_picture;

		if(ret < 0){
			printf("Decode Error.\n");
			//av_free_packet(input->in_packet);
			return NULL;
		}
		if(got_picture){
			sws_scale(input->img_convert_ctx, (const unsigned char* const*)input->pFrame->data, input->pFrame->linesize, 0, input->pCodecCtx->height, ost->frame->data,  ost->frame->linesize);
			//ost->frame->pts =ost->next_pts;
			//ost->frame->pts =pts;
			ost->frame->pts =av_rescale_q_rnd(pts, input->v_ifmtCtx->streams[input->videoindex]->time_base, ost->enc->time_base,AV_ROUND_UP);
			ret_frame= ost->frame;

			if (debug_ts) {
				av_log(NULL, AV_LOG_INFO,"222 pts:%s  video frame_pts:%s  frame_pts_time:%s \n",
				av_ts2str(pts),av_ts2str(ost->frame->pts), av_ts2timestr(ost->frame->pts,&ost->enc->time_base));
		    			
			}
		}
	}
	return ret_frame;
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
	video_input.bCap=1;

//******************************//
}


int videoThreadProc(void *arg)
{
	int got_pic;
	while(video_input.bCap)
	{


		AVPacket * pkt=get_video_pkt(output_dev.video_st,&video_input);

		if(pkt==NULL)
		{
			//packet_queue_put_nullpacket(&output_dev.videoq,0);
			video_input.bCap =0;

		}
		else
		{
			packet_queue_put(&output_dev.videoq,pkt,output_dev.video_st->next_pts);
		}


#if 0
		AVFrame *vframe=get_video_frame1(output_dev.video_st,&video_input,&got_pic);
		if(!got_pic)
		{
			usleep(10000);
			continue;

		}
		video_input.bCap = !write_video_frame1(output_dev.oc, output_dev.video_st,vframe);
		//printf("videoThreadProc runing video_input.bCap=%d\n",video_input.bCap);
#endif 
	}

	printf("videoThreadProc exit\n");
	usleep(1000000);
	return 0;
	
}



