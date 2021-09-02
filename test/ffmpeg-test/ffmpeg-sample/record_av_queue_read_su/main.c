#include "common.h"
 
extern IntputDev alsa_input;
extern IntputViDev video_input ;

extern OutputDev output_dev;
extern AVPacket flush_pkt;

int video_queue_handle(int64_t *frame_pts)
{
	AVFrame *vframe;
	AVPacket pkt;
	int got_pic;

	if (debug_ts) {
		printf("nb_packets %d video_input.bCap=%d frame_pts=%s \n",output_dev.videoq.nb_packets,video_input.bCap,
							av_ts2str(*frame_pts));

	}
	if( output_dev.videoq.nb_packets==0)
	{
		if(video_input.bCap==0)
		{
			printf("exit while\n");
			vframe=NULL;
			
			goto WRITE_FRAME;//need to flush encoder

			//output_dev.encode_video=0;
			//continue;
			//break;
		}
		else
		{
			return 1;
		}
		
	}

	if(packet_queue_get(&output_dev.videoq,&pkt,0,frame_pts)<0)
	{
		printf("packet_queue_get Error.\n");
		return -1;
	}

	if(flush_pkt.data== pkt.data)
	{
		printf("get pkt flush_pkt\n");
			return 1;
	}


	  vframe=get_video_pkt2Frame(output_dev.video_st,&video_input,&pkt,&got_pic,*frame_pts);
	if(!got_pic)
	{
		av_free_packet(&pkt);
		printf("get_video_pkt2Frame error\n");
		usleep(200);
			return 1;
	}
	av_free_packet(&pkt);			

WRITE_FRAME:
	output_dev.encode_video = !write_video_frame1(output_dev.oc, output_dev.video_st,vframe);
	//usleep(300000);
	return 0;
}

int audio_queue_handle(int64_t *frame_audio_pts)
{
	int got_pcm;
	AVFrame *aframe;
	AVPacket audio_pkt;
	if (debug_ts) {
		printf("nb_packets %d  frame_audio_pts=%s\n",output_dev.audioq.nb_packets,av_ts2str(*frame_audio_pts));
	}
	if( output_dev.audioq.nb_packets==0)
	{
		if(alsa_input.bCap==0)
		{
			printf("exit while\n");
			aframe=NULL;
			goto WRITE_AUDIO_FRAME;//need to flush encoder

			//output_dev.encode_audio=0;
			//return -1;
			//break;

		}
		else
		{
			return 1;
		}
		
	}

	if(packet_queue_get(&output_dev.audioq,&audio_pkt,0,frame_audio_pts)<0)
	{
		printf("packet_queue_get Error.\n");
			return -1;
	}

	if(flush_pkt.data== audio_pkt.data)
	{
		printf("get pkt flush_pkt\n");
		return 1;
	}
	//av_free_packet(&audio_pkt);			

#if 1


	  aframe=get_audio_pkt2Frame(output_dev.audio_st,&alsa_input,&audio_pkt,&got_pcm,*frame_audio_pts);
	if(!got_pcm)
	{
		av_free_packet(&audio_pkt);
		printf("get_video_pkt2Frame error\n");
		usleep(10000);
		return 1;
	}
	av_free_packet(&audio_pkt);			

WRITE_AUDIO_FRAME:
	output_dev.encode_audio = !write_audio_frame1(output_dev.oc, output_dev.audio_st,aframe);
	//usleep(300000);
	
#endif
	return 0;
}

int main(int argc, char **argv)
{
	const char *filename;
	AVDictionary *opt = NULL;
	int i;
	int ret;


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

	av_init_packet(&flush_pkt);
    	flush_pkt.data = (uint8_t *)&flush_pkt;

	open_audio_capture();
	open_video_capture();

	open_output(filename,opt);


	if(ret=capture_start(&output_dev.videoq,videoThreadProc,"video_capture",NULL)<0)
		goto quit;

	if(ret=capture_start(&output_dev.audioq,audioThreadProc,"audio_capture",NULL)<0)
		goto quit;

	
 	printf("init finish\n");

	int64_t frame_pts=1,frame_audio_pts=1;

    while (output_dev.encode_video || output_dev.encode_audio) { //){//
        /* select the stream to encode */

		//output_dev.encode_video=0;

		//printf(" frame_pts=%s audio_st->next_pts=%s\n",av_ts2str(frame_pts),av_ts2str(output_dev.audio_st->next_pts));

		if (output_dev.encode_video &&
		(!output_dev.encode_audio || frame_pts<=frame_audio_pts ) )
		{
			ret=video_queue_handle(&frame_pts);
			if(ret==-1)
				break;
			else if(ret==1)
			{
				usleep(200);					
				continue;
			}
			
		}
	 	else//audio
	 	{
	 		ret=audio_queue_handle(&frame_audio_pts);
			if(ret==-1)
				break;
			else if(ret==1)
			{
				usleep(200);					
				continue;
			}

		}
		
		

		


    }

quit:
    av_write_trailer(output_dev.oc);

//audio 
 	printf("free audio\n");

	swr_free(&alsa_input.audio_convert_ctx); 
	avcodec_close(alsa_input.pCodecCtx);	
	av_free(alsa_input.pAudioFrame);
	av_free(alsa_input.dst_buffer); 
	avformat_close_input(&alsa_input.a_ifmtCtx);
	if(&output_dev.audioq)
	{
		packet_queue_destroy(&output_dev.audioq);
	}

//vidoe 

	usleep(100000);

 	printf("free video\n");

	sws_freeContext(video_input.img_convert_ctx);
	avcodec_close(video_input.pCodecCtx);
	av_free(video_input.pFrameYUV);
	av_free(video_input.pFrame);	
	avformat_close_input(&video_input.v_ifmtCtx);

	if(&output_dev.videoq)
	{
		packet_queue_destroy(&output_dev.videoq);
	}

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


