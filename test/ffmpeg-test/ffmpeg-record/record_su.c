
 
/**
 * 最简单的基于FFmpeg的AVDevice例子（读取摄像头）
 * Simplest FFmpeg Device (Read Camera)
 *
 * 雷霄骅 Lei Xiaohua
 * leixiaohua1020@126.com
 * 中国传媒大学/数字电视技术
 * Communication University of China / Digital TV Technology
 * http://blog.csdn.net/leixiaohua1020
 *
 * 本程序实现了本地摄像头数据的获取解码和显示。是基于FFmpeg
 * 的libavdevice类库最简单的例子。通过该例子，可以学习FFmpeg中
 * libavdevice类库的使用方法。
 * 本程序在Windows下可以使用2种方式读取摄像头数据：
 *  1.VFW: Video for Windows 屏幕捕捉设备。注意输入URL是设备的序号，
 *          从0至9。
 *  2.dshow: 使用Directshow。注意作者机器上的摄像头设备名称是
 *         “Integrated Camera”，使用的时候需要改成自己电脑上摄像头设
 *          备的名称。
 * 在Linux下则可以使用video4linux2读取摄像头设备。
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
	
	gRecordContext->FrameCount = 0;						//????????Ч???????
	gRecordContext->IsShow = IsShow;
	gRecordContext->device_name = "/dev/video0";
	
	av_register_all();					//????????е????????????y?????
	avformat_network_init();			//???????y?????????Э??
	
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
	//????????????????Щ????
 
	av_dict_set(&option, "video_size", "640x480", 0);		//???÷????
	av_dict_set(&option, "pixel_format", "mjpeg", 0);	
	//------------------------------------------------------------------------------	
	/*
	????input_fmt???豸???"/dev/video0"?????pInFmtContext????
	??????????pInFmtContext???????/dev/video0?豸??ifmt???????????????????
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
 
 
	//????????????е??????????
	ret = avformat_find_stream_info(gRecordContext->pInFmtContext,NULL);
	ERROR(ret < 0, err8, "Couldn't find stream information.\n");	
	
	//????????????е???????????????videoindex
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
	
	//????????????????????洢??????????и???????
	gRecordContext->pOutCodecContext = gRecordContext->out_vd_stream->codec;
	
	gRecordContext->pOutCodecContext->codec_id = gRecordContext->output_fmt->video_codec;		//??????ID
	gRecordContext->pOutCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;						//IO???????????????????
	gRecordContext->pOutCodecContext->pix_fmt = PIX_FMT_YUV420P;								//???????????
	gRecordContext->pOutCodecContext->width = gRecordContext->pInCodecContext->width;  		//????????????????????
	gRecordContext->pOutCodecContext->height = gRecordContext->pInCodecContext->height;		//??????????????????????
	gRecordContext->pOutCodecContext->time_base = gRecordContext->out_vd_stream->time_base;
	gRecordContext->pOutCodecContext->bit_rate = 400000;							//??????
	gRecordContext->pOutCodecContext->gop_size=250;								//????GOP??С???250????????I?
	
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
	//?????Щ????
 
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
	???????е?????????????????????????????YUV420P???????????豸????????????????????????
	?????libswscale???????и????????????????YUV420P?????????б????????
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
	
		
	/*?洢?????????????????????塣AVFrame????????*/
	gRecordContext->in_packet 	= (AVPacket *)av_malloc(sizeof(AVPacket));	
	
	//Be care full of these: av_new_packet should be call
	gRecordContext->out_packet 	= (AVPacket *)av_malloc(sizeof(AVPacket));	 	//20150918
	av_new_packet(gRecordContext->out_packet, gRecordContext->OutFrameBufSize);	//20150918
	
	//img_convert_context??????????????????Э?飺????????????????...
	gRecordContext->img_convert_context = sws_getContext(gRecordContext->pInCodecContext->width,	\
	gRecordContext->pInCodecContext->height, gRecordContext->pInCodecContext->pix_fmt,	\
	gRecordContext->pOutCodecContext->width, gRecordContext->pOutCodecContext->height,	\
	gRecordContext->pOutCodecContext->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL); 
 
	
	gRecordContext->pix_size = gRecordContext->pOutCodecContext->width * gRecordContext->pOutCodecContext->height;
	
	//???????????д??header
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
	/*????????е???????????????????????磬????????????
	?????????????????????? av_read_frame()?????????????????????
	??????????????н???????H.264????????????????????NAL????
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
			
			//???????????????н??????????
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
				???????е?????????????????????????????YUV420P???????????豸????????????????????????
				?????libswscale???????и????????????????YUV420P?????????б????????
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
 
					//重要！！！！时间戳		
					av_packet_rescale_ts(gRecordContext->out_packet, gRecordContext->pOutCodecContext->time_base, gRecordContext->out_vd_stream->time_base);	
					
					LOGI("after rescale: PTS = %d\t DTS = %d\n", gRecordContext->out_packet->pts, gRecordContext->out_packet->dts);
					LOGI("after rescale: duration = %d\t convergence_duration = %d\n", gRecordContext->out_packet->duration, gRecordContext->out_packet->convergence_duration);
					
					gRecordContext->out_packet->stream_index = gRecordContext->out_vd_stream->index;		//????????/?????: ???
					
					//???????д?????????
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
	//???????????????????????????????У???д??????????????
	ret = flush_encoder(gRecordContext->pOutFmtContext, gRecordContext->out_vd_stream->index);
	if (ret < 0) 
	{
		LOGE("Flushing encoder failed\n");
	}
	
	LOGI("av_write_trailer\n");
	//???????????д??tail
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
	
	int				videoindex;			//输入视频流的序号
	AVFormatContext	*pInFmtContext;		//输入码流的上下文属性描述		
	AVCodecContext	*pInCodecContext;	//输入视频流的编码信息
	AVCodec			*pInCodec;			//输入视频流需要的解码器
	AVInputFormat 	*input_fmt;			//输入视频流格式
	
	AVFormatContext* pOutFmtContext;		//输出码流的上下文属性描述	
	AVCodecContext* pOutCodecContext;		//输出视频流的编码信息
	AVCodec* pOutCodec;						//输出视频流需要的编码器	
	AVOutputFormat* output_fmt;				//输出视频流格式
	
	AVStream* out_vd_stream;				//AVStream是存储每一个视频/音频流信息的结构体
	AVPacket out_packet;					//压缩数据包
	
	
	const char* out_file = "luo.mp4";
	
	av_register_all();					//初始化所有的编解码器，复用解复用器
	avformat_network_init();			//初始化流媒体网络相关协议
	
	//分配空间
	pInFmtContext = avformat_alloc_context();
	
	//分配空间
	pOutFmtContext = avformat_alloc_context();
	
	//初始化libavdevice库
	avdevice_register_all();
 
    //寻找video4linux2的视频流输入格式 	
	input_fmt = av_find_input_format("video4linux2");
	
	//根据输出文件名取得文件编码格式，也就是视频流输出格式
	output_fmt = av_guess_format(NULL, out_file, NULL);
	pOutFmtContext->oformat = output_fmt;
	
	/*
	根据input_fmt和设备文件"/dev/video0"初始化pInFmtContext码流
	可以理解为，pInFmtContext码流是从/dev/video0设备以ifmt的格式读出来的视频流，
	avformat_open_input第四个参数option是对input_fmt格式的操作，如分辨率；NULL——>不操作
	*/
	if(avformat_open_input(&pInFmtContext, "/dev/video0", input_fmt, NULL)!=0){
		printf("Couldn't open input stream.\n");
		return -1;
	}
	
	/*
	打开输出流out_file（与打开输入流类似）
	根据上面取得的output_fmt以及out_file的形式（可以是RMTP/UDP/TCP/file）初始化输出码流
	*/
	if (avio_open(&pOutFmtContext->pb, out_file, AVIO_FLAG_READ_WRITE) < 0){
		printf("Failed to open output file! \n");
		return -1;
	}
 
 
	//查询输入码流中的所有流信息
	if(avformat_find_stream_info(pInFmtContext,NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}
	
	//寻找输入码流中的视频流，保存序号videoindex
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
	
	//在输出码流中新建一个视频流
	out_vd_stream = avformat_new_stream(pOutFmtContext, 0);
	out_vd_stream->time_base.num = 1; 
	out_vd_stream->time_base.den = 25; 
	if (out_vd_stream == NULL){
		return -1;
	}	
	
	//取出输入视频流的编码信息
	pInCodecContext=pInFmtContext->streams[videoindex]->codec;
	printf("--line %d--in_w = %d\t in_h = %d't fmt = %d\n", __LINE__, pInCodecContext->width, pInCodecContext->height, pInCodecContext->pix_fmt);
 
	//根据编码信息里面的编码器ID，找到对应的解码器
	pInCodec=avcodec_find_decoder(pInCodecContext->codec_id);
	if(pInCodec==NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}
	
	//打开并初始化pInCodec解码器
	if(avcodec_open2(pInCodecContext, pInCodec,NULL)<0)
	{
		printf("Could not open codec.\n");
		return -1;
	}
	printf("--line %d--in_w = %d\t in_h = %d't fmt = %d\n", __LINE__, pInCodecContext->width, pInCodecContext->height, pInCodecContext->pix_fmt);
 
	//获取视频流的编码信息存储地址，然后进行赋值初始化
	pOutCodecContext = out_vd_stream->codec;
	
	pOutCodecContext->codec_id = output_fmt->video_codec;		//编码器ID
	pOutCodecContext->codec_type = AVMEDIA_TYPE_VIDEO;			//IO流类型：视频流，音频流
	pOutCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;				//视频流的帧格式
	pOutCodecContext->width = pInCodecContext->width;  			//帧宽（使用输入视频流的帧宽）
	pOutCodecContext->height = pInCodecContext->height;			//帧高（使用输入视频流的帧高）
	pOutCodecContext->time_base.num = 1;  
	pOutCodecContext->time_base.den = 25;  						//设置帧率25
	pOutCodecContext->bit_rate = 400000;						//比特率  
	pOutCodecContext->gop_size=250;								//设置GOP大小：每250帧插入一个I帧
	//H264
	//pOutCodecContext->me_range = 16;
	//pOutCodecContext->max_qdiff = 4;
	//pOutCodecContext->qcompress = 0.6;
	pOutCodecContext->qmin = 10;
	pOutCodecContext->qmax = 51;
 
	//Optional Param
	//该值表示在两个非B帧之间，允许插入的B帧的最大帧数
	pOutCodecContext->max_b_frames=3;	
 
	//Show some Information
	//将输出码流的信息显示到终端
	av_dump_format(pOutFmtContext, 0, out_file, 1);
	
	//根据解码器ID找到对应的解码器
	pOutCodec = avcodec_find_encoder(pOutCodecContext->codec_id);
	if (!pOutCodec)
	{
		printf("Can not find encoder! \n");
		return -1;
	}
	
	//------------------------------------------------------------------------------
	//设置一些参数
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
	//打开并初始化pOutCodec解码器
	if (avcodec_open2(pOutCodecContext, pOutCodec, param) < 0)
	{
		printf("Failed to open encoder! \n");
		return -1;
	}
	//------------------------------------------------------------------------------
	
	
	/*
	存储：
	1. 原始数据（即非压缩数据，例如对视频来说是YUV，RGB，对音频来说是PCM）
	2. 帧信息
	*/
	AVFrame	*pInFrame;				//输入的视频流中取出来的视频帧
	AVFrame	*pOutFrame;				//转换成YUV420P格式后的视频帧
	
	//初始化帧
	pInFrame = av_frame_alloc();
	pOutFrame = av_frame_alloc();
	
	/*
	由于pInFrame是来自于视频流，解码函数会自动为其分配帧数据空间
	而pFrameYUV是转换格式后的帧，需要预先分配空间给它
	**注意:
	几乎所有的编码器编码用的数据源格式都必须是YUV420P，所以当视频设备取出来的帧格式不是这个格式时，
	需要用libswscale库来进行格式和分辨率的转换，至YUV420P之后，才能进行编码压缩。
	解码过程：AVStream --> AVPacket --> AVFrame（AVFrame是非压缩数据包，可直接用于显示）
	编码过程：AVFrame --> AVPacket --> AVStream
	*/
	int buf_size;
	uint8_t* out_buf;
	
	//avpicture_get_size(目标格式，目标帧宽，目标帧高)
	buf_size = avpicture_get_size(pOutCodecContext->pix_fmt, pOutCodecContext->width, pOutCodecContext->height);
	out_buf = (uint8_t *)av_malloc(buf_size);
	avpicture_fill((AVPicture *)pOutFrame, out_buf, pOutCodecContext->pix_fmt, pOutCodecContext->width, pOutCodecContext->height);
	
	
	int ret, got_picture;
	
	/*存储压缩编码数据相关信息的结构体。AVFrame是非压缩的*/
	AVPacket *in_packet=(AVPacket *)av_malloc(sizeof(AVPacket));
	
	//根据Frame大小，初始化一个Packet
	av_new_packet(&out_packet,buf_size);
	
	//往输出码流中写入header
	avformat_write_header(pOutFmtContext,NULL);
  
	/*
	libswscale是一个主要用于处理图片像素数据的类库。可以完成图片像素格式的转换，图片的拉伸等工作
	sws_getContext()：初始化一个SwsContext。
	sws_scale()：处理图像数据。
	sws_freeContext()：释放一个SwsContext。
	其中sws_getContext()也可以用sws_getCachedContext()取代。几乎“万能”的图片像素数据处理类库
	
	srcW：源图像的宽
	srcH：源图像的高
	srcFormat：源图像的像素格式
	dstW：目标图像的宽
	dstH：目标图像的高
	dstFormat：目标图像的像素格式
	flags：设定图像拉伸使用的算法
	成功执行的话返回生成的SwsContext，否则返回NULL。
	*/
	struct SwsContext *img_convert_context;
	
	//img_convert_context描述了两个格式转换的协议：格式、分辨率、转码算法...
	img_convert_context = sws_getContext(pInCodecContext->width, pInCodecContext->height, pInCodecContext->pix_fmt, pOutCodecContext->width, pOutCodecContext->height, pOutCodecContext->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL); 
 
	int pix_size;
	pix_size = pOutCodecContext->width * pOutCodecContext->height;
	
	//================================================
	int framenum=500;
	int framecnt=0;	//Frames to encode 经过编码的有效帧数
 
	for (;;) 
	{
		/*读取码流中的音频若干帧或者视频一帧。例如，解码视频的时候，
		每解码一个视频帧，需要先调用 av_read_frame()获得一个视频的压缩数据包，
		然后才能对该数据进行解码（例如H.264中一帧压缩数据通常对应一个NAL）。
		packet是压缩数据
		*/
		if(av_read_frame(pInFmtContext, in_packet)>=0)
		{
			//如果该数据包是视频数据包，则进行视频解码
			if(in_packet->stream_index == videoindex)
			{
				/*
				从packet压缩数据里取出一帧AVFrame非压缩数据
				输入一个压缩编码的结构体AVPacket，输出一个解码后的结构体AVFrame
				*/
				ret = avcodec_decode_video2(pInCodecContext, pInFrame, &got_picture, in_packet);
				if(ret < 0)
				{
					printf("Decode Error.\n");
					av_free_packet(in_packet);
					continue;
				}
				
				//成功从输入视频流中解码出一帧数据
				if(got_picture)
				{				
					//转换帧格式
					sws_scale(img_convert_context, (const uint8_t* const*)pInFrame->data, pInFrame->linesize, 0, pInCodecContext->height, pOutFrame->data, pOutFrame->linesize);
 
					//PTS: 帧时间戳
					pOutFrame->pts = framecnt;
					framecnt++;
					if(framecnt > framenum)
					{
						printf("framecnt > %d \n", framenum);
						av_free_packet(in_packet);
						break;
					}
					
					//开始压缩数据
					got_picture = 0;
					/*
					将帧编码成包：输入一个帧，输出一个包
					**注意:
					几乎所有的编码器编码用的数据源格式都必须是YUV420P，所以当视频设备取出来的帧格式不是这个格式时，
					需要用libswscale库来进行格式和分辨率的转换，至YUV420P之后，才能进行编码压缩。
					解码过程：AVStream --> AVPacket --> AVFrame（AVFrame是非压缩数据包，可直接用于显示）
					编码过程：AVFrame --> AVPacket --> AVStream
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
												
						out_packet.stream_index = out_vd_stream->index;		//标识该视频/音频流: 序号
						
						//将视频包写入到输出码流
						ret = av_write_frame(pOutFmtContext, &out_packet);
						
						//释放该包
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
	//将编码器中剩余的帧数据冲刷到输出码流中，即写入文件，防止丢失帧
	ret = flush_encoder(pOutFmtContext, 0);
	if (ret < 0) 
	{
		printf("Flushing encoder failed\n");
	}
	
	printf("av_write_trailer\n");
	//往输出码流中写入tail
	av_write_trailer(pOutFmtContext);
 
	sws_freeContext(img_convert_context);
 
 
	//Clean
	if (out_vd_stream)
	{
		//关闭输出视频流的编码器
		avcodec_close(pOutCodecContext);
		//释放帧
		av_free(pOutFrame);
		//释放缓存
		av_free(out_buf);
	}
	//关闭输出视频流
	avio_close(pOutFmtContext->pb);
	//关闭输出码流
	avformat_free_context(pOutFmtContext);
 
 
	//av_free(out_buffer);
	av_free(pInFrame);
	//关闭输入视频流的解码器
	avcodec_close(pInCodecContext);
	
	//关闭输入码流
	avformat_close_input(&pInFmtContext);
 
	return 0;
}

 
#ifdef __cplusplus
};
#endif
