
 
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
 
#define STREAM_FRAME_RATE 25 // 25 fps
 
 
 
static struct RecordContext *gRecordContext;		
 
static int flush_encoder(AVFormatContext *fmt_context, unsigned int stream_index)
{
	int ret;
	int got_picture;
	AVPacket encode_packet;
	
	if (!(fmt_context->streams[stream_index]->codec->codec->capabilities & AV_CODEC_CAP_DELAY))
		return 0;
	
	while (1) 
	{
		encode_packet.data = NULL;
		encode_packet.size = 0;
		av_init_packet(&encode_packet);
		
		ret = avcodec_encode_video2 (fmt_context->streams[stream_index]->codec, &encode_packet,
			NULL, &got_picture);
			
		av_frame_free(NULL);
		if (ret < 0)
			break;
		if (!got_picture)
		{
			ret=0;
			break;
		}
		LOGI("Flush Encoder: Succeed to encode 1 frame!\tsize:%5d\n", encode_packet.size);
		/* mux encoded frame */
		ret = av_write_frame(fmt_context, &encode_packet);
		if (ret < 0)
			break;
	}
	return ret;
}
 
int SetShowBufferAddr(unsigned int *ShowBuffer)
{
	//????????
	if(NULL == ShowBuffer)
	{
		return -1;
	}
	else
	{
		gRecordContext->ShowBuffer = ShowBuffer;
	}
 
	return 0;
}
 
 
int RecordInit(char *RecordFile, int IsShow)
{
	int i;
	int ret = -1;
	char file[24] = {0};
	AVDictionary *option = NULL;
	AVDictionary *param = NULL;
	
	gRecordContext = (struct RecordContext *)calloc(1, sizeof(struct RecordContext));
	ERROR(NULL == gRecordContext, err1, "calloc gRecordContext");
	
	//memset(gRecordContext, 0, sizeof(struct RecordContext));
	LOGI("RecordInit start\n");
	
	
	//?????????????
	if(RecordFile == NULL)
	{
		LOGI("create a random file to record video\n");		
		srand((unsigned)time(NULL));
		sprintf(file, "/storage/sdcard0/Download/0917-%d-%d.mp4", rand()%10, rand()%10);
		gRecordContext->out_file_name = file;
	}
	else
	{
		gRecordContext->out_file_name = RecordFile;
	}
	
	gRecordContext->FrameCount = 0;						//????????��???????
	gRecordContext->IsShow = IsShow;
	gRecordContext->device_name = "/dev/video0";
	
	av_register_all();					//????????��????????????y?????
	avformat_network_init();			//???????y?????????��??
	
	//??????
	gRecordContext->pInFmtContext = avformat_alloc_context();
	
	//??????
	gRecordContext->pOutFmtContext = avformat_alloc_context();
	ERROR(((gRecordContext->pInFmtContext == NULL) || (gRecordContext->pOutFmtContext == NULL)), err2, "avformat_alloc_context");
 
	//?????libavdevice??
	avdevice_register_all();
 
    //???video4linux2?????????????
	gRecordContext->input_fmt = av_find_input_format("video4linux2");
	ERROR((gRecordContext->input_fmt == NULL), err3, "Couldn't av_find_input_format\n");
	
	//??????????????????????????????????????????
	gRecordContext->output_fmt = av_guess_format(NULL, gRecordContext->out_file_name, NULL);
	gRecordContext->pOutFmtContext->oformat = gRecordContext->output_fmt;
	
	//------------------------------------------------------------------------------
	//????????????????��????
 
	av_dict_set(&option, "video_size", "640x480", 0);		//???��????
	av_dict_set(&option, "pixel_format", "mjpeg", 0);	
	//------------------------------------------------------------------------------	
	/*
	????input_fmt???��???"/dev/video0"?????pInFmtContext????
	??????????pInFmtContext???????/dev/video0?��??ifmt???????????????????
	avformat_open_input?????????option???input_fmt?????????????????NULL????>??????
	*/
	ret = access(gRecordContext->device_name, F_OK);
	ERROR(ret < 0, err3, "device is not exsist!\n")
	
	ret = avformat_open_input(&gRecordContext->pInFmtContext, "/dev/video0", gRecordContext->input_fmt, NULL);
	ERROR((ret != 0), err3, "Couldn't open input stream.\n");
 
	/*
	???????out_file????????????????
	????????????output_fmt???out_file?????????????RMTP/UDP/TCP/file??????????????
	*/
	ret = avio_open(&gRecordContext->pOutFmtContext->pb, gRecordContext->out_file_name, AVIO_FLAG_READ_WRITE);
	ERROR(ret < 0, err7, "Failed to open output file! \n");
 
 
	//????????????��??????????
	ret = avformat_find_stream_info(gRecordContext->pInFmtContext,NULL);
	ERROR(ret < 0, err8, "Couldn't find stream information.\n");	
	
	//????????????��???????????????videoindex
	gRecordContext->videoindex = -1;
	for(i = 0; i < gRecordContext->pInFmtContext->nb_streams; i++)
	{
		if(gRecordContext->pInFmtContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			gRecordContext->videoindex = i;
			break;
		}		
	}
		
	ERROR((gRecordContext->videoindex == -1), err9, "Couldn't find a video stream.\n");
	
	
	//??????????????????????
	gRecordContext->out_vd_stream = avformat_new_stream(gRecordContext->pOutFmtContext, 0);
	ERROR((gRecordContext->out_vd_stream == NULL), err10, "avformat_new_stream");
	
 
	gRecordContext->out_vd_stream->time_base = (AVRational){1, STREAM_FRAME_RATE}; 
		
	
	//????????????????????
	gRecordContext->pInCodecContext = gRecordContext->pInFmtContext->streams[gRecordContext->videoindex]->codec;
	LOGI("--line %d-- in_w = %d\t in_h = %d\t in_fmt = %d\t in_encode = %d\n",	\
	__LINE__, gRecordContext->pInCodecContext->width, gRecordContext->pInCodecContext->height,	\
	gRecordContext->pInCodecContext->pix_fmt, gRecordContext->pInCodecContext->codec_id);
 
	//????????????????????ID???????????????
	gRecordContext->pInCodec = avcodec_find_decoder(gRecordContext->pInCodecContext->codec_id);
	ERROR((gRecordContext->pInCodec == NULL), err11, "Codec not found.\n");	
	
	//?????????pInCodec??????
	ret = avcodec_open2(gRecordContext->pInCodecContext, gRecordContext->pInCodec,NULL);
	ERROR(ret < 0, err12, "Could not open input codec.\n")
	
	LOGI("--line %d-- in_w = %d\t in_h = %d\t in_fmt = %d\t in_encode = %d\n",	\
	__LINE__, gRecordContext->pInCodecContext->width, gRecordContext->pInCodecContext->height,	\
	gRecordContext->pInCodecContext->pix_fmt, gRecordContext->pInCodecContext->codec_id);
	
	//????????????????????��??????????��???????
	gRecordContext->pOutCodecContext = gRecordContext->out_vd_stream->codec;
	
	gRecordContext->pOutCodecContext->codec_id = gRecordContext->output_fmt->video_codec;		//??????ID
	gRecordContext->pOutCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;						//IO???????????????????
	gRecordContext->pOutCodecContext->pix_fmt = PIX_FMT_YUV420P;								//???????????
	gRecordContext->pOutCodecContext->width = gRecordContext->pInCodecContext->width;  		//????????????????????
	gRecordContext->pOutCodecContext->height = gRecordContext->pInCodecContext->height;		//??????????????????????
	gRecordContext->pOutCodecContext->time_base = gRecordContext->out_vd_stream->time_base;
	gRecordContext->pOutCodecContext->bit_rate = 400000;							//??????
	gRecordContext->pOutCodecContext->gop_size=250;								//????GOP??��???250????????I?
	
	LOGI("--line %d-- out_w = %d\t out_h = %d\t out_fmt = %d\t out_encode = %d\n",	\
	__LINE__, gRecordContext->pOutCodecContext->width, gRecordContext->pOutCodecContext->height,	\
	gRecordContext->pOutCodecContext->pix_fmt, gRecordContext->pOutCodecContext->codec_id);
 
	//H264
	//pOutCodecContext->me_range = 16;
	//pOutCodecContext->max_qdiff = 4;
	//pOutCodecContext->qcompress = 0.6;
	gRecordContext->pOutCodecContext->qmin = 10;
	gRecordContext->pOutCodecContext->qmax = 51;
 
	//Optional Param
	//??????????????B????????????B?????????
	gRecordContext->pOutCodecContext->max_b_frames=3;
 
	/* Some formats want stream headers to be separate. */
    if (gRecordContext->pOutFmtContext->oformat->flags & AVFMT_GLOBALHEADER)
	{
		LOGI("AVFMT_GLOBALHEADER\n");
		gRecordContext->pOutCodecContext->flags |= CODEC_FLAG_GLOBAL_HEADER;		
	}
        	
 
	//Show some Information
	//???????????????????????1,??????????; 0, ???????????
	av_dump_format(gRecordContext->pOutFmtContext, 0, gRecordContext->out_file_name, 1);
	
	//?????????ID?????????????
	gRecordContext->pOutCodec = avcodec_find_encoder(gRecordContext->pOutCodecContext->codec_id);
	ERROR(!gRecordContext->pOutCodec, err13, "Can not find encoder! \n");
		
	//------------------------------------------------------------------------------
	//?????��????
 
	//H.264
	if(gRecordContext->pOutCodecContext->codec_id == AV_CODEC_ID_H264) {
		av_dict_set(?m, "preset", "slow", 0);
		av_dict_set(?m, "tune", "zerolatency", 0);
		av_dict_set(?m, "profile", "main", 0);
	}
	//H.265
	if(gRecordContext->pOutCodecContext->codec_id == AV_CODEC_ID_H265){
		av_dict_set(?m, "preset", "ultrafast", 0);
		av_dict_set(?m, "tune", "zero-latency", 0);
	}
	
	//20150929
	LOGI("extradata_size = %d\n", gRecordContext->pOutCodecContext->extradata_size);
	if(gRecordContext->pOutCodecContext->extradata_size > 0)
	{
		printf_array(gRecordContext->pOutCodecContext->extradata, gRecordContext->pOutCodecContext->extradata_size);
	}
	
	//?????????pOutCodec??????
	if (avcodec_open2(gRecordContext->pOutCodecContext, gRecordContext->pOutCodec, ?m) < 0)
	{
		LOGE("Failed to open encoder! \n");
		return -1;
	}
	
	//20150929
	LOGI("extradata_size = %d\n", gRecordContext->pOutCodecContext->extradata_size);
	if(gRecordContext->pOutCodecContext->extradata_size > 0)
	{
		printf_array(gRecordContext->pOutCodecContext->extradata, gRecordContext->pOutCodecContext->extradata_size);
	}
	//------------------------------------------------------------------------------
	
	//??????
	gRecordContext->pInFrame = av_frame_alloc();
	//avpicture_get_size(???????????????????)
	gRecordContext->InFrameBufSize = avpicture_get_size(gRecordContext->pInCodecContext->pix_fmt,	\
	gRecordContext->pInCodecContext->width, gRecordContext->pInCodecContext->height);
	LOGI("ShowBufferSize = InFrameBufSize = %d\n", gRecordContext->InFrameBufSize);
	
 
	gRecordContext->ShowBufferSize = gRecordContext->InFrameBufSize;
 
	
	gRecordContext->pOutFrame = av_frame_alloc();
	
	/*
	????pInFrame???????????????????????????????????????
	??pOutFrame????????????????????????????
	**???:
	???????��?????????????????????????????YUV420P???????????��????????????????????????
	?????libswscale???????��????????????????YUV420P?????????��????????
	????????AVStream --> AVPacket --> AVFrame??AVFrame???????????????????????????
	????????AVFrame --> AVPacket --> AVStream
	*/	
	//avpicture_get_size(???????????????????)
	gRecordContext->OutFrameBufSize = avpicture_get_size(gRecordContext->pOutCodecContext->pix_fmt,	\
	gRecordContext->pOutCodecContext->width, gRecordContext->pOutCodecContext->height);
	LOGI("OutFrameBufSize = %d\n", gRecordContext->OutFrameBufSize);
	
	gRecordContext->OutFrameBuffer = (uint8_t *)av_malloc(gRecordContext->OutFrameBufSize);
	avpicture_fill((AVPicture *)gRecordContext->pOutFrame, gRecordContext->OutFrameBuffer, gRecordContext->pOutCodecContext->pix_fmt,	\
	gRecordContext->pOutCodecContext->width, gRecordContext->pOutCodecContext->height);
	
		
	/*?��?????????????????????�^AVFrame????????*/
	gRecordContext->in_packet 	= (AVPacket *)av_malloc(sizeof(AVPacket));	
	
	//Be care full of these: av_new_packet should be call
	gRecordContext->out_packet 	= (AVPacket *)av_malloc(sizeof(AVPacket));	 	//20150918
	av_new_packet(gRecordContext->out_packet, gRecordContext->OutFrameBufSize);	//20150918
	
	//img_convert_context??????????????????��?�????????????????...
	gRecordContext->img_convert_context = sws_getContext(gRecordContext->pInCodecContext->width,	\
	gRecordContext->pInCodecContext->height, gRecordContext->pInCodecContext->pix_fmt,	\
	gRecordContext->pOutCodecContext->width, gRecordContext->pOutCodecContext->height,	\
	gRecordContext->pOutCodecContext->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL); 
 
	
	gRecordContext->pix_size = gRecordContext->pOutCodecContext->width * gRecordContext->pOutCodecContext->height;
	
	//???????????��??header
	avformat_write_header(gRecordContext->pOutFmtContext,NULL);
	
	LOGI("RecordInit end\n");
 
	return gRecordContext->ShowBufferSize;
 
err1:
	return -1;
 
err2:
	LOGI("err2 11111\n");
	FREE(gRecordContext);
	LOGI("err2 22222\n");
	return -1;
err3:
	LOGI("err3 11111\n");
	avformat_free_context(gRecordContext->pOutFmtContext);
	avformat_free_context(gRecordContext->pInFmtContext);
	FREE(gRecordContext);
	
	LOGI("err3 22222\n");
	return -1;
	
err7:
err8:
err9:
err10:
err11:
err12:
err13:
	LOGI("err3 11111\n");
	avformat_close_input(&gRecordContext->pInFmtContext);
	avformat_free_context(gRecordContext->pOutFmtContext);
	avformat_free_context(gRecordContext->pInFmtContext);
	FREE(gRecordContext);
	
	LOGI("err3 22222\n");
	return -1;	
	
 
}
 
int Recording(void)
{
	int ret = -1;
	int got_picture = -1;
	
	LOGI("Recording start\n");
	/*????????��???????????????????????��????????????
	?????????????????????? av_read_frame()?????????????????????
	??????????????��???????H.264????????????????????NAL????
	packet?????????
	*/
	ret = av_read_frame(gRecordContext->pInFmtContext, gRecordContext->in_packet);
	if(ret >= 0)
	{
		LOGI("lines= %d\tfunc = %s, frame count: %5d\n", __LINE__, __func__, gRecordContext->FrameCount);
 
		//??????????????????????????????????
		if(gRecordContext->in_packet->stream_index == gRecordContext->videoindex)
		{
			LOGI("lines= %d\tfunc = %s, frame count: %5d\n", __LINE__, __func__, gRecordContext->FrameCount);
			
			/*
			??packet??????????????AVFrame?????????
			??????????????????AVPacket?????????????????AVFrame
			*/
			ret = avcodec_decode_video2(gRecordContext->pInCodecContext, gRecordContext->pInFrame, &got_picture, gRecordContext->in_packet);
			if(ret < 0)
			{
				LOGE("Decode Error.\n");
				av_free_packet(gRecordContext->in_packet);
				return -1;
			}
			LOGI("lines= %d\tfunc = %s, frame count: %5d\n", __LINE__, __func__, gRecordContext->FrameCount);
			
			//???????????????��??????????
			if(got_picture == 1)
			{				
				LOGI("lines= %d\tfunc = %s, frame count: %5d\n", __LINE__, __func__, gRecordContext->FrameCount);
				
				if(1 == gRecordContext->IsShow)
				{
					//?????????????????????????apk?????
					memcpy(gRecordContext->ShowBuffer, (const uint8_t* const*)gRecordContext->pInFrame->data, gRecordContext->ShowBufferSize);
				}
				
				LOGI("lines= %d\tfunc = %s, frame count: %5d\n", __LINE__, __func__, gRecordContext->FrameCount);
				
				//???????
				sws_scale(gRecordContext->img_convert_context, (const uint8_t* const*)gRecordContext->pInFrame->data, gRecordContext->pInFrame->linesize, 0, gRecordContext->pInCodecContext->height, gRecordContext->pOutFrame->data, gRecordContext->pOutFrame->linesize);
				LOGI("lines= %d\tfunc = %s, frame count: %5d\n", __LINE__, __func__, gRecordContext->FrameCount);
				
				
				//PTS: ?????
				gRecordContext->pOutFrame->pts = gRecordContext->FrameCount;
				gRecordContext->FrameCount++;	
				
				if (gRecordContext->pOutFmtContext->oformat->flags & AVFMT_RAWPICTURE)	
				{
					LOGI("raw picture\n");
				}
				
				//??????????
				got_picture = 0;
				av_init_packet(gRecordContext->out_packet);
				/*
				?????????????????????????????
				**???:
				???????��?????????????????????????????YUV420P???????????��????????????????????????
				?????libswscale???????��????????????????YUV420P?????????��????????
				????????AVStream --> AVPacket --> AVFrame??AVFrame???????????????????????????
				????????AVFrame --> AVPacket --> AVStream
				*/
				ret = avcodec_encode_video2(gRecordContext->pOutCodecContext, gRecordContext->out_packet, gRecordContext->pOutFrame, &got_picture);
				if(ret < 0)
				{
					LOGE("Failed to encode! \n");
					av_free_packet(gRecordContext->in_packet);
					return -1;
				}
				LOGI("lines= %d\tfunc = %s, frame count: %5d\n", __LINE__, __func__, gRecordContext->FrameCount);
 
				if (got_picture == 1)
				{
					LOGI("Succeed to encode frame: %5d\tsize:%5d \tindex = %d\n", gRecordContext->FrameCount, gRecordContext->out_packet->size, gRecordContext->out_vd_stream->index);
					
					LOGI("before rescale: PTS = %d\t DTS = %d\n", gRecordContext->out_packet->pts, gRecordContext->out_packet->dts);
					LOGI("before rescale: duration = %d\t convergence_duration = %d\n", gRecordContext->out_packet->duration, gRecordContext->out_packet->convergence_duration);
 
					//��Ҫ��������ʱ���		
					av_packet_rescale_ts(gRecordContext->out_packet, gRecordContext->pOutCodecContext->time_base, gRecordContext->out_vd_stream->time_base);	
					
					LOGI("after rescale: PTS = %d\t DTS = %d\n", gRecordContext->out_packet->pts, gRecordContext->out_packet->dts);
					LOGI("after rescale: duration = %d\t convergence_duration = %d\n", gRecordContext->out_packet->duration, gRecordContext->out_packet->convergence_duration);
					
					gRecordContext->out_packet->stream_index = gRecordContext->out_vd_stream->index;		//????????/?????: ???
					
					//???????��?????????
					ret = av_write_frame(gRecordContext->pOutFmtContext, gRecordContext->out_packet);
					
					//???e?
					av_free_packet(gRecordContext->out_packet);
				}
			}
		}
		av_free_packet(gRecordContext->in_packet);
	}
 
	LOGI("Recording end\n");
	
	return 0;
}
 
static int printf_array(unsigned int *array, int size)
{
	int i;
	for(i = 0; i < size; i++)
	{
		printf("0x%x  ", *(array + i));
	}
	printf("\n");
}
 
int RecordUninit(void)
{	
	int ret = -1;
 
	LOGI("RecordUninit start\n");
	//=========================================
	//???????????????????????????????��???��??????????????
	ret = flush_encoder(gRecordContext->pOutFmtContext, gRecordContext->out_vd_stream->index);
	if (ret < 0) 
	{
		LOGE("Flushing encoder failed\n");
	}
	
	LOGI("av_write_trailer\n");
	//???????????��??tail
	av_write_trailer(gRecordContext->pOutFmtContext);
 
	sws_freeContext(gRecordContext->img_convert_context);
 
 
	//Clean
	if (gRecordContext->out_vd_stream)
	{
		//??????????????????
		avcodec_close(gRecordContext->pOutCodecContext);
		
		//????
		av_frame_free(&gRecordContext->pOutFrame);		
		
		//????		
		av_free(gRecordContext->out_packet);
		
		//??????
		av_free(gRecordContext->OutFrameBuffer);		
	}
	
	//???????????
	avio_close(gRecordContext->pOutFmtContext->pb);
	//??????????
	avformat_free_context(gRecordContext->pOutFmtContext);
 
 
	//????	
	av_frame_free(&gRecordContext->pInFrame);
	
	//????
	//av_free_packet(gRecordContext->in_packet);
	av_free(gRecordContext->in_packet);
	
	//???????????????????
	avcodec_close(gRecordContext->pInCodecContext);
	
	//???????????
	avformat_close_input(&gRecordContext->pInFmtContext);
	avformat_free_context(gRecordContext->pInFmtContext);
	
	FREE(gRecordContext);
	
	LOGI("RecordUninit end\n");
 
	return 0;
}


  
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
	pOutCodecContext->gop_size=250;								//����GOP��С��ÿ250֡����һ��I֡
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
	
	//------------------------------------------------------------------------------
	//����һЩ����
	AVDictionary *param = 0;
	//H.264
	if(pOutCodecContext->codec_id == AV_CODEC_ID_H264) {
		av_dict_set(param, "preset", "slow", 0);
		av_dict_set(param, "tune", "zerolatency", 0);
		//av_dict_set(param, "profile", "main", 0);
	}
	//H.265
	if(pOutCodecContext->codec_id == AV_CODEC_ID_HEVC){
		av_dict_set(param, "preset", "ultrafast", 0);
		av_dict_set(param, "tune", "zero-latency", 0);
	}
	//�򿪲���ʼ��pOutCodec������
	if (avcodec_open2(pOutCodecContext, pOutCodec, param) < 0)
	{
		printf("Failed to open encoder! \n");
		return -1;
	}
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

 
#ifdef __cplusplus
};
#endif
