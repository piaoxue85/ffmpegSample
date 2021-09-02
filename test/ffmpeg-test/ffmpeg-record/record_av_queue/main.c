#include "common.h"
 
extern IntputDev alsa_input;
extern IntputViDev video_input ;

extern OutputDev output_dev;
extern AVPacket flush_pkt;


int main(int argc, char **argv)
{
	const char *filename;
	AVDictionary *opt = NULL;
	int i;
	int got_pcm,got_pic,ret;


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

	
 	printf("init finish\n");

	int64_t frame_pts=1;
    while (output_dev.encode_video || output_dev.encode_audio) { //){//
        /* select the stream to encode */

		//output_dev.encode_audio=0;

		printf(" frame_pts=%s audio_st->next_pts=%s\n",av_ts2str(frame_pts),av_ts2str(output_dev.audio_st->next_pts));

		if (output_dev.encode_video &&
		(!output_dev.encode_audio || av_compare_ts(frame_pts, output_dev.video_st->enc->time_base,
		        output_dev.audio_st->next_pts, output_dev.audio_st->enc->time_base) <= 0)) 
		{
			AVPacket pkt;
			//if(output_dev.videoq.nb_packets==0)
			//printf("nb_packets %d video_input.bCap=%d frame_pts=%s \n",output_dev.videoq.nb_packets,video_input.bCap,
			//av_ts2str(frame_pts));

			if( output_dev.videoq.nb_packets==0)
			{
				if(video_input.bCap==0)
				{
					printf("exit while\n");
					output_dev.encode_video=0;
					continue;
					//break;
				}
				usleep(50000);
				continue;
			}
			
			if(packet_queue_get(&output_dev.videoq,&pkt,0,&frame_pts)<0)
			{
				printf("packet_queue_get Error.\n");
				break;
			}

			if(flush_pkt.data== pkt.data)
			{
				printf("get pkt flush_pkt\n");
				continue;
			}

			ret = write_frame(output_dev.oc, &output_dev.video_st->enc->time_base, output_dev.video_st->st, &pkt);
			if (ret < 0) {
			    fprintf(stderr, "Error while writing video frame: %s\n", av_err2str(ret));
			}
			
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

quit:
    av_write_trailer(output_dev.oc);

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


