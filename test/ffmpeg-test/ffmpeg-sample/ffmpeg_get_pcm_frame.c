 
/**
  *
 * This software read data from Computer's Camera and play it.
 * It's the simplest example about usage of FFmpeg's libavdevice Library. 
 * It's suiltable for the beginner of FFmpeg.
 * This software support 2 methods to read camera in Microsoft Windows:
 *  1.gdigrab: VfW (Video for Windows) capture input device.
 *             The filename passed as input is the capture driver number,
 *             ranging from 0 to 9.
 *  2.dshow: Use Directshow. Camera's name in author's computer is 
 *             "Integrated Camera".
 * It use video4linux2 to read Camera in Linux.
 * It use avfoundation to read Camera in MacOS.
 * 
 */
 
 
#include <stdio.h>
 

//Linux...
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <SDL/SDL.h>
#ifdef __cplusplus
};
#endif
 
//Output PCM 
#define OUTPUT_PCM 1

#define SWR_NEW 1
int thread_exit=0;
 #define MAX_AUDIO_FRAME_SIZE 192000

 
int main(int argc, char* argv[])
{
 
	AVFormatContext	*pFormatCtx;
	int				i, videoindex;
	AVCodecContext	*pCodecCtx;
	AVCodec			*pCodec;
	
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();
	
	//Register Device
	avdevice_register_all();
	

    //Linux
	AVInputFormat *ifmt=av_find_input_format("alsa");
	if(avformat_open_input(&pFormatCtx,"default",ifmt,NULL)!=0){
		printf("Couldn't open input stream.default\n");
		return -1;
	}
 
 
	if(avformat_find_stream_info(pFormatCtx,NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	videoindex=-1;
	for(i=0; i<pFormatCtx->nb_streams; i++) 
		if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO)
		{
			videoindex=i;
			break;
		}
	if(videoindex==-1)
	{
		printf("Couldn't find a video stream.\n");
		return -1;
	}
	pCodecCtx=pFormatCtx->streams[videoindex]->codec;
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
			 
	int ret, got_audio;
 
	AVPacket *packet=(AVPacket *)av_malloc(sizeof(AVPacket));

	AVFrame* pAudioFrame=av_frame_alloc();
	if(NULL==pAudioFrame)
	{
		printf("could not alloc pAudioFrame\n");
		return -1;
	}

//audio output paramter //resample 
	uint64_t out_channel_layout = AV_CH_LAYOUT_MONO;
	int out_sample_fmt = AV_SAMPLE_FMT_S16;
	int out_nb_samples =1024; //pCodecCtx->frame_size;
	int out_sample_rate = 44100;
	int out_nb_channels = av_get_channel_layout_nb_channels(out_channel_layout);
	int out_buffer_size = av_samples_get_buffer_size(NULL, out_nb_channels, out_nb_samples, out_sample_fmt, 1);  
	uint8_t *buffer=NULL;  
	buffer = (uint8_t *)av_malloc(MAX_AUDIO_FRAME_SIZE); 
	int64_t in_channel_layout = av_get_default_channel_layout(pCodecCtx->channels);  

	printf("audio sample_fmt=%d size=%d channel=%d in_channel_layout=%d sample_rate=%d\n",pCodecCtx->sample_fmt, pCodecCtx->frame_size,
		pCodecCtx->channels,in_channel_layout,pCodecCtx->sample_rate);

	struct SwrContext   *audio_convert_ctx = NULL;  
	audio_convert_ctx = swr_alloc();  
	if (audio_convert_ctx == NULL)  
	{  
	    printf("Could not allocate SwrContext\n");  
	    return -1;  
	}  

#if SWR_NEW

	int64_t src_ch_layout = AV_CH_LAYOUT_STEREO, dst_ch_layout = AV_CH_LAYOUT_STEREO;
	int src_rate = 48000, dst_rate = 44100;
	uint8_t **src_data = NULL, **dst_data = NULL;
	int src_nb_channels = 0, dst_nb_channels = 0;
	int src_linesize, dst_linesize;
	int src_nb_samples = 1024, dst_nb_samples, max_dst_nb_samples;
	enum AVSampleFormat src_sample_fmt = AV_SAMPLE_FMT_DBL, dst_sample_fmt = AV_SAMPLE_FMT_S16;
	const char *dst_filename = NULL;
	FILE *dst_file;
	int dst_bufsize;
	const char *fmt;

	 src_ch_layout= in_channel_layout;
	 src_rate=pCodecCtx->sample_rate;
	 src_sample_fmt= pCodecCtx->sample_fmt;
	// dst_ch_layout=out_nb_channels;
	// dst_rate=out_sample_rate;
	// dst_sample_fmt=out_sample_fmt;
		/* set options */

	printf("dst_sample_fmt %d dst_ch_layout=%s src_sample_fmt=%d src_ch_layout=%s\n",
	dst_sample_fmt,av_ts2str(dst_ch_layout),src_sample_fmt,av_ts2str(src_ch_layout));


    av_opt_set_int(audio_convert_ctx, "in_channel_layout",    src_ch_layout, 0);
    av_opt_set_int(audio_convert_ctx, "in_sample_rate",       src_rate, 0);
    av_opt_set_sample_fmt(audio_convert_ctx, "in_sample_fmt", src_sample_fmt, 0);

    av_opt_set_int(audio_convert_ctx, "out_channel_layout",    dst_ch_layout, 0);
    av_opt_set_int(audio_convert_ctx, "out_sample_rate",       dst_rate, 0);
    av_opt_set_sample_fmt(audio_convert_ctx, "out_sample_fmt", dst_sample_fmt, 0);

        /* initialize the resampling context */
    if ((ret = swr_init(audio_convert_ctx)) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");

    }

    /* allocate source and destination samples buffers */

    src_nb_channels = av_get_channel_layout_nb_channels(src_ch_layout);
    ret = av_samples_alloc_array_and_samples(&src_data, &src_linesize, src_nb_channels,
                                             src_nb_samples, src_sample_fmt, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate source samples\n");

    }

    /* compute the number of converted samples: buffering is avoided
     * ensuring that the output buffer will contain at least all the
     * converted input samples */
    max_dst_nb_samples = dst_nb_samples =
        av_rescale_rnd(src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);

    /* buffer is going to be directly written to a rawaudio file, no alignment */
    dst_nb_channels = av_get_channel_layout_nb_channels(dst_ch_layout);
    ret = av_samples_alloc_array_and_samples(&dst_data, &dst_linesize, dst_nb_channels,
                                             dst_nb_samples, dst_sample_fmt, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate destination samples\n");

    }
	
	
#else


	if((ret=swr_alloc_set_opts(audio_convert_ctx, out_channel_layout, out_sample_fmt,\
		out_sample_rate,in_channel_layout, \
		pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0, NULL))<0){
		  printf("Could not swr_alloc_set_opts\n");  
       	 return -1;  
	}

	 if ((ret = swr_init(audio_convert_ctx)) < 0) {
            fprintf(stderr, "Failed to initialize the resampling context\n");
            exit(1);
        }
        
#endif

	
	int frameCnt=200;
#if OUTPUT_PCM 
    FILE *fp_pcm=fopen("output.pcm","wb+");  
#endif 

	while(frameCnt--){
		if(av_read_frame(pFormatCtx, packet)>=0){
		if(packet->stream_index==videoindex){
			ret = avcodec_decode_audio4(pCodecCtx, pAudioFrame, &got_audio, packet);
			if(ret < 0){
				printf("Decode Error.\n");
				return -1;
			}
			if(got_audio){

				printf("nb_samples %d out_buffer_size=%d\n",pAudioFrame->nb_samples,out_buffer_size);
#if SWR_NEW

		        /* compute destination number of samples */
		      /*  dst_nb_samples = av_rescale_rnd(swr_get_delay(audio_convert_ctx, src_rate) +
		                                        src_nb_samples, dst_rate, src_rate, AV_ROUND_UP);

				printf("src_rate= %d  src_nb_samples=%d dst_rate=%d dst_nb_samples=%d\n",
					src_rate,src_nb_samples,dst_rate,dst_nb_samples);

		        if (dst_nb_samples > max_dst_nb_samples) {
		            av_freep(&dst_data[0]);
		            ret = av_samples_alloc(dst_data, &dst_linesize, dst_nb_channels,
		                                   dst_nb_samples, dst_sample_fmt, 1);
		            if (ret < 0)
		                break;
		            max_dst_nb_samples = dst_nb_samples;
		        }*/



		        /* convert to destination format */
		        ret = swr_convert(audio_convert_ctx, dst_data, dst_nb_samples, (const uint8_t **)pAudioFrame->data, pAudioFrame->nb_samples);
		        if (ret < 0) {
		            fprintf(stderr, "Error while converting\n");

		        }
		        dst_bufsize = av_samples_get_buffer_size(&dst_linesize, dst_nb_channels,
		                                                 ret, dst_sample_fmt, 1);
		        if (dst_bufsize < 0) {
		            fprintf(stderr, "Could not get sample buffer size\n");

		        }
		        printf(" in:%d out:%d dst_bufsize=%d\n", src_nb_samples, ret,dst_bufsize);
#if OUTPUT_PCM  
			fwrite(dst_data[0],1,dst_bufsize,fp_pcm);    //Y   
		
#endif 		

#else		
			swr_convert(audio_convert_ctx, &buffer, out_buffer_size, (const uint8_t **)pAudioFrame->data, pAudioFrame->nb_samples);  

#if OUTPUT_PCM  
			fwrite(buffer,1,out_buffer_size,fp_pcm);    //Y   
		
#endif 
#endif			  

			}
		}
		av_free_packet(packet);
				
		}
	}

 #if OUTPUT_PCM
    fclose(fp_pcm);
#endif 


#if SWR_NEW

 if (src_data)
        av_freep(&src_data[0]);
    av_freep(&src_data);
	

 if (dst_data)
        av_freep(&dst_data[0]);
    av_freep(&dst_data);

#endif	
	swr_free(&audio_convert_ctx); 
	av_free(buffer); 
	//av_free(out_buffer);
	av_free(pAudioFrame);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
 
	return 0;
}
