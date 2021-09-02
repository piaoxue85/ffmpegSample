/**
 * ��򵥵Ļ���FFmpeg��AVDevice���ӣ���ȡ����ͷ��
 * Simplest FFmpeg Device (Read Camera)
 *
 * ������ Lei Xiaohua
 * leixiaohua1020@126.com
 * �й���ý��ѧ/���ֵ��Ӽ���
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * ������ʵ���˱�������ͷ���ݵĻ�ȡ�������ʾ���ǻ���FFmpeg
 * ��libavdevice�����򵥵����ӡ�ͨ�������ӣ�����ѧϰFFmpeg��
 * libavdevice����ʹ�÷�����
 * ��������Windows�¿���ʹ��2�ַ�ʽ��ȡ����ͷ���ݣ�
 *  1.VFW: Video for Windows ��Ļ��׽�豸��ע������URL���豸����ţ�
 *          ��0��9��
 *  2.dshow: ʹ��Directshow��ע�����߻����ϵ�����ͷ�豸������
 *         ��Integrated Camera����ʹ�õ�ʱ����Ҫ�ĳ��Լ�����������ͷ��
 *          �������ơ�
 * ��Linux�������ʹ��video4linux2��ȡ����ͷ�豸��
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
 * 
 */
 
 
 
#define __STDC_CONSTANT_MACROS	
 
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
 
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <SDL/SDL.h>
 
#ifdef __cplusplus
};
#endif
 
 
 
static int flush_encoder(AVFormatContext *fmt_context, unsigned int stream_index);
 
int main(int argc, char* argv[])
{
	int				i;
	
	int				videoindex;			//������Ƶ�������
	AVFormatContext	*pInFmtContext;		//������������������������		
	AVCodecContext	*pInCodecContext;	//������Ƶ���ı�����Ϣ
	AVCodec			*pInCodec;			//������Ƶ����Ҫ�Ľ�����
	AVInputFormat 	*input_fmt;			//������Ƶ����ʽ
	
	AVFormatContext* pOutFmtContext;		//�����������������������	
	AVCodecContext* pOutCodecContext;		//�����Ƶ���ı�����Ϣ
	AVCodec* pOutCodec;						//�����Ƶ����Ҫ�ı�����	
	AVOutputFormat* output_fmt;				//�����Ƶ����ʽ
	
	AVStream* out_vd_stream;				//AVStream�Ǵ洢ÿһ����Ƶ/��Ƶ����Ϣ�Ľṹ��
	AVPacket out_packet;					//ѹ�����ݰ�
	
	
	const char* out_file = "luo.mp4";
	
	av_register_all();					//��ʼ�����еı�����������ý⸴����
	avformat_network_init();			//��ʼ����ý���������Э��
	
	//����ռ�
	pInFmtContext = avformat_alloc_context();
	
	//����ռ�
	pOutFmtContext = avformat_alloc_context();
	
	//��ʼ��libavdevice��
	avdevice_register_all();
 
    //Ѱ��video4linux2����Ƶ�������ʽ 	
	input_fmt = av_find_input_format("video4linux2");
	
	//��������ļ���ȡ���ļ������ʽ��Ҳ������Ƶ�������ʽ
	output_fmt = av_guess_format(NULL, out_file, NULL);
	pOutFmtContext->oformat = output_fmt;
	
	/*
	����input_fmt���豸�ļ�"/dev/video0"��ʼ��pInFmtContext����
	�������Ϊ��pInFmtContext�����Ǵ�/dev/video0�豸��ifmt�ĸ�ʽ����������Ƶ����
	avformat_open_input���ĸ�����option�Ƕ�input_fmt��ʽ�Ĳ�������ֱ��ʣ�NULL����>������
	*/
	if(avformat_open_input(&pInFmtContext, "/dev/video0", input_fmt, NULL)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}
	
	/*
	�������out_file��������������ƣ�
	��������ȡ�õ�output_fmt�Լ�out_file����ʽ��������RMTP/UDP/TCP/file����ʼ���������
	*/
	if (avio_open(&pOutFmtContext->pb, out_file, AVIO_FLAG_READ_WRITE) < 0){
		printf("Failed to open output file! \n");
		return -1;
	}
 
 
	//��ѯ���������е���������Ϣ
	if(avformat_find_stream_info(pInFmtContext,NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	
	//Ѱ�����������е���Ƶ�����������videoindex
	videoindex=-1;
	for(i=0; i<pInFmtContext->nb_streams; i++) 
		if(pInFmtContext->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO)
		{
			videoindex=i;
			break;
		}
	if(videoindex==-1)
	{
		printf("Couldn't find a video stream.\n");
		return -1;
	}
	
	//������������½�һ����Ƶ��
	out_vd_stream = avformat_new_stream(pOutFmtContext, 0);
	out_vd_stream->time_base.num = 1; 
	out_vd_stream->time_base.den = 25; 
	if (out_vd_stream == NULL){
		return -1;
	}	
	
	//ȡ��������Ƶ���ı�����Ϣ
	pInCodecContext=pInFmtContext->streams[videoindex]->codec;
	printf("--line %d--in_w = %d\t in_h = %d't fmt = %d\n", __LINE__, pInCodecContext->width, pInCodecContext->height, pInCodecContext->pix_fmt);
 
	//���ݱ�����Ϣ����ı�����ID���ҵ���Ӧ�Ľ�����
	pInCodec=avcodec_find_decoder(pInCodecContext->codec_id);
	if(pInCodec==NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	
	//�򿪲���ʼ��pInCodec������
	if(avcodec_open2(pInCodecContext, pInCodec,NULL)<0)
	{
		printf("Could not open codec.\n");
		return -1;
	}
	printf("--line %d--in_w = %d\t in_h = %d't fmt = %d\n", __LINE__, pInCodecContext->width, pInCodecContext->height, pInCodecContext->pix_fmt);

	//��ȡ��Ƶ���ı�����Ϣ�洢��ַ��Ȼ����и�ֵ��ʼ��
	pOutCodecContext = out_vd_stream->codec;
	
	pOutCodecContext->codec_id = output_fmt->video_codec;		//������ID
	pOutCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;			//IO�����ͣ���Ƶ������Ƶ��
	pOutCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;				//��Ƶ����֡��ʽ
	pOutCodecContext->width = pInCodecContext->width;  			//֡��ʹ��������Ƶ����֡��
	pOutCodecContext->height = pInCodecContext->height;			//֡�ߣ�ʹ��������Ƶ����֡�ߣ�
	pOutCodecContext->time_base.num = 1;  
	pOutCodecContext->time_base.den = 25;  						//����֡��25
	pOutCodecContext->bit_rate = 400000;						//������  
	pOutCodecContext->gop_size=10;								//����GOP��С��ÿ250֡����һ��I֡
	//H264
	//pOutCodecContext->me_range = 16;
	//pOutCodecContext->max_qdiff = 4;
	//pOutCodecContext->qcompress = 0.6;
	pOutCodecContext->qmin = 10;
	pOutCodecContext->qmax = 51;
 
	//Optional Param
	//��ֵ��ʾ��������B֮֡�䣬��������B֡�����֡��
	pOutCodecContext->max_b_frames=3;	
 
	//Show some Information
	//�������������Ϣ��ʾ���ն�
	av_dump_format(pOutFmtContext, 0, out_file, 1);
	
	//���ݽ�����ID�ҵ���Ӧ�Ľ�����


	pOutCodec = avcodec_find_encoder(pOutCodecContext->codec_id);
	if (!pOutCodec)
	{
		printf("Can not find encoder! \n");
		return -1;
	}

		printf("avcodec_find_encoder\n");

	printf("pOutCodecContext->codec_id= %d  AV_CODEC_ID_H264=%d\n", __LINE__, pOutCodecContext->codec_id,AV_CODEC_ID_H264);

	//------------------------------------------------------------------------------
	//����һЩ����
	AVDictionary *param = 0;
	//H.264
	if(pOutCodecContext->codec_id == AV_CODEC_ID_H264) {


		av_dict_set(&param, "preset", "slow", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);
		//av_dict_set(param, "profile", "main", 0);
	}
	//H.265
	if(pOutCodecContext->codec_id == AV_CODEC_ID_HEVC){

		printf("AV_CODEC_ID_HEVC av_dict_set\n");

		av_dict_set(&param, "preset", "ultrafast", 0);
		av_dict_set(&param, "tune", "zero-latency", 0);
	}

	//�򿪲���ʼ��pOutCodec������
	if (avcodec_open2(pOutCodecContext, pOutCodec, &param) < 0)
	{
		printf("Failed to open encoder! \n");
		return -1;
	}


	printf("avcodec_open2\n");

	//------------------------------------------------------------------------------
	
	
	/*
	�洢��
	1. ԭʼ���ݣ�����ѹ�����ݣ��������Ƶ��˵��YUV��RGB������Ƶ��˵��PCM��
	2. ֡��Ϣ
	*/
	AVFrame	*pInFrame;				//�������Ƶ����ȡ��������Ƶ֡
	AVFrame	*pOutFrame;				//ת����YUV420P��ʽ�����Ƶ֡
	
	//��ʼ��֡
	pInFrame = av_frame_alloc();
	pOutFrame = av_frame_alloc();
	
	/*
	����pInFrame����������Ƶ�������뺯�����Զ�Ϊ�����֡���ݿռ�
	��pFrameYUV��ת����ʽ���֡����ҪԤ�ȷ���ռ����
	**ע��:
	�������еı����������õ�����Դ��ʽ��������YUV420P�����Ե���Ƶ�豸ȡ������֡��ʽ���������ʽʱ��
	��Ҫ��libswscale�������и�ʽ�ͷֱ��ʵ�ת������YUV420P֮�󣬲��ܽ��б���ѹ����
	������̣�AVStream --> AVPacket --> AVFrame��AVFrame�Ƿ�ѹ�����ݰ�����ֱ��������ʾ��
	������̣�AVFrame --> AVPacket --> AVStream
	*/
	int buf_size;
	uint8_t* out_buf;
	
	//avpicture_get_size(Ŀ���ʽ��Ŀ��֡��Ŀ��֡��)
	buf_size = avpicture_get_size(pOutCodecContext->pix_fmt, pOutCodecContext->width, pOutCodecContext->height);
	out_buf = (uint8_t *)av_malloc(buf_size);
	avpicture_fill((AVPicture *)pOutFrame, out_buf, pOutCodecContext->pix_fmt, pOutCodecContext->width, pOutCodecContext->height);
	
	
	int ret, got_picture;
	
	/*�洢ѹ���������������Ϣ�Ľṹ�塣AVFrame�Ƿ�ѹ����*/
	AVPacket *in_packet=(AVPacket *)av_malloc(sizeof(AVPacket));
	
	//����Frame��С����ʼ��һ��Packet
	av_new_packet(&out_packet,buf_size);
	
	//�����������д��header
	avformat_write_header(pOutFmtContext,NULL);


	printf("avformat_write_header");
  
	/*
	libswscale��һ����Ҫ���ڴ���ͼƬ�������ݵ���⡣�������ͼƬ���ظ�ʽ��ת����ͼƬ������ȹ���
	sws_getContext()����ʼ��һ��SwsContext��
	sws_scale()������ͼ�����ݡ�
	sws_freeContext()���ͷ�һ��SwsContext��
	����sws_getContext()Ҳ������sws_getCachedContext()ȡ�������������ܡ���ͼƬ�������ݴ������
	
	srcW��Դͼ��Ŀ�
	srcH��Դͼ��ĸ�
	srcFormat��Դͼ������ظ�ʽ
	dstW��Ŀ��ͼ��Ŀ�
	dstH��Ŀ��ͼ��ĸ�
	dstFormat��Ŀ��ͼ������ظ�ʽ
	flags���趨ͼ������ʹ�õ��㷨
	�ɹ�ִ�еĻ��������ɵ�SwsContext�����򷵻�NULL��
	*/
	struct SwsContext *img_convert_context;
	
	//img_convert_context������������ʽת����Э�飺��ʽ���ֱ��ʡ�ת���㷨...
	img_convert_context = sws_getContext(pInCodecContext->width, pInCodecContext->height, pInCodecContext->pix_fmt, pOutCodecContext->width, pOutCodecContext->height, pOutCodecContext->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL); 
 
	int pix_size;
	pix_size = pOutCodecContext->width * pOutCodecContext->height;
	
	//================================================
	int framenum=500;
	int framecnt=0;	//Frames to encode �����������Ч֡��

	printf("before av_read_frame\n");

	for (;;) 
	{
		/*��ȡ�����е���Ƶ����֡������Ƶһ֡�����磬������Ƶ��ʱ��
		ÿ����һ����Ƶ֡����Ҫ�ȵ��� av_read_frame()���һ����Ƶ��ѹ�����ݰ���
		Ȼ����ܶԸ����ݽ��н��루����H.264��һ֡ѹ������ͨ����Ӧһ��NAL����
		packet��ѹ������
		*/
		if(av_read_frame(pInFmtContext, in_packet)>=0)
		{
			//��������ݰ�����Ƶ���ݰ����������Ƶ����
			if(in_packet->stream_index == videoindex)
			{
				/*
				��packetѹ��������ȡ��һ֡AVFrame��ѹ������
				����һ��ѹ������Ľṹ��AVPacket�����һ�������Ľṹ��AVFrame
				*/
				ret = avcodec_decode_video2(pInCodecContext, pInFrame, &got_picture, in_packet);
				if(ret < 0)
				{
					printf("Decode Error.\n");
					av_free_packet(in_packet);
					continue;
				}
				
				//�ɹ���������Ƶ���н����һ֡����
				if(got_picture)
				{				
					//ת��֡��ʽ
					sws_scale(img_convert_context, (const uint8_t* const*)pInFrame->data, pInFrame->linesize, 0, pInCodecContext->height, pOutFrame->data, pOutFrame->linesize);
 
					//PTS: ֡ʱ���
					pOutFrame->pts = framecnt;
					framecnt++;
					if(framecnt > framenum)
					{
						printf("framecnt > %d \n", framenum);
						av_free_packet(in_packet);
						break;
					}

					pOutFrame->width=pInFrame->width;
					pOutFrame->height=pInFrame->height;
					pOutFrame->format=pInFrame->format;

					//printf("before pInFrame w=%d h=%d format=%d linesize=%d\n",pInFrame->width,pInFrame->height,pInFrame->format,pInFrame->linesize);
					//printf("before pOutFrame w=%d h=%d linesize=%d\n",pOutFrame->width,pOutFrame->height,pOutFrame->linesize);

					
					//��ʼѹ������
					got_picture = 0;
					/*
					��֡����ɰ�������һ��֡�����һ����
					**ע��:
					�������еı����������õ�����Դ��ʽ��������YUV420P�����Ե���Ƶ�豸ȡ������֡��ʽ���������ʽʱ��
					��Ҫ��libswscale�������и�ʽ�ͷֱ��ʵ�ת������YUV420P֮�󣬲��ܽ��б���ѹ����
					������̣�AVStream --> AVPacket --> AVFrame��AVFrame�Ƿ�ѹ�����ݰ�����ֱ��������ʾ��
					������̣�AVFrame --> AVPacket --> AVStream
					*/

					av_init_packet(&out_packet);
					
					ret = avcodec_encode_video2(pOutCodecContext, &out_packet, pOutFrame, &got_picture);
					if(ret < 0)
					{
						printf("Failed to encode! \n");
						av_free_packet(in_packet);
						continue;
					}
					if (got_picture == 1)
					{
						printf("Succeed to encode frame: %5d\tsize:%5d \tindex = %d\n", framecnt, out_packet.size, out_vd_stream->index);
												
						out_packet.stream_index = out_vd_stream->index;		//��ʶ����Ƶ/��Ƶ��: ���


						av_packet_rescale_ts(&out_packet, pOutCodecContext->time_base, out_vd_stream->time_base);	

						//����Ƶ��д�뵽�������
						ret = av_write_frame(pOutFmtContext, &out_packet);
						
						//�ͷŸð�
						av_free_packet(&out_packet);
					}
				}
			}
			av_free_packet(in_packet);
		}
		else
		{
			
			break;
		}
	}
	
	//=========================================
	//����������ʣ���֡���ݳ�ˢ����������У���д���ļ�����ֹ��ʧ֡
	ret = flush_encoder(pOutFmtContext, 0);
	if (ret < 0) 
	{
		printf("Flushing encoder failed\n");
	}
	
	printf("av_write_trailer\n");
	//�����������д��tail
	av_write_trailer(pOutFmtContext);
 
	sws_freeContext(img_convert_context);
 
 
	//Clean
	if (out_vd_stream)
	{
		//�ر������Ƶ���ı�����
		avcodec_close(pOutCodecContext);
		//�ͷ�֡
		av_free(pOutFrame);
		//�ͷŻ���
		av_free(out_buf);
	}
	//�ر������Ƶ��
	avio_close(pOutFmtContext->pb);
	//�ر��������
	avformat_free_context(pOutFmtContext);
 
 
	//av_free(out_buffer);
	av_free(pInFrame);
	//�ر�������Ƶ���Ľ�����
	avcodec_close(pInCodecContext);
	
	//�ر���������
	avformat_close_input(&pInFmtContext);
 
	return 0;
}
 
 
static int flush_encoder(AVFormatContext *fmt_context, unsigned int stream_index)
{
	int ret;
	int got_picture;
	AVPacket enc_packet;
	
	if (!(fmt_context->streams[stream_index]->codec->codec->capabilities & AV_CODEC_CAP_DELAY))
		return 0;
	
	while (1) 
	{
		enc_packet.data = NULL;
		enc_packet.size = 0;
		av_init_packet(&enc_packet);
		
		ret = avcodec_encode_video2 (fmt_context->streams[stream_index]->codec, &enc_packet,
			NULL, &got_picture);
			
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_picture)
		{
			ret=0;
			break;
		}
		printf("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", enc_packet.size);
		/* mux encoded frame */
		ret = av_write_frame(fmt_context, &enc_packet);
		if (ret < 0)
			break;
	}
	return ret;
}
