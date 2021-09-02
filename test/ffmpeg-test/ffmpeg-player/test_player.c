/* 
 * A simple player with FFMPEG4.0 and SDL2.0.8. 
 * Only support video decoder, not support audio and subtitle. 
 * Created by LiuWei@20180524 
 * Reference: https://blog.csdn.net/leixiaohua1020/article/details/38868499 
 */  
#include <libavcodec/avcodec.h>  
#include <libavformat/avformat.h>  
#include <libswscale/swscale.h>  
#include <SDL2/SDL.h>  
#include <libavutil/imgutils.h>  
   
/* Output YUV420P data as a file 
 * 0 means close */  
#define OUTPUT_YUV420P 0  
   
int main(int argc, char *argv[])  
{  
    /* AVFormatContext contains: 
     * 1. iformat(AVInputFormat) : It's either autodetected or set by user. 
     * 2. oformat(AVOutputFormat): always set by user for output. 
     * 3. streams "array" of AVStreams: describe all elementary streams stored in the file. 
     *    AVStreams are typically referred to using their index in this array. */  
    /* We can keep pFormatCtx as NULL. avformat_open_input() will allocate for it. */  
    AVFormatContext *pFormatCtx = NULL;  
   
    int             i, videoindex, audioindex, titleindex;  
    AVCodecContext  *pCodecCtx;  
    AVCodec         *pCodec;  
    AVFrame         *pFrame, *pFrameYUV;  
    AVStream    *avStream;  
    AVPacket        *packet;  
    unsigned char   *out_buffer;      
        int             y_size;  
        int             ret;  
    struct SwsContext *img_convert_ctx;  
   
    char filepath[256] = {0};  
    /* SDL */  
    int screen_w, screen_h = 0;  
    SDL_Window   *screen;  
    SDL_Renderer *sdlRenderer;  
    SDL_Texture  *sdlTexture;  
    SDL_Rect     sdlRect;  
   
    FILE *fp_yuv;  
   
    /* Parse input parameter. The right usage: ./myplayer xxx.mp4 */  
    if(argc < 2 || argc > 2) {  
        printf("Too few or too many parameters. e.g. ./myplayer xxx.mp4\n");  
        return -1;  
    }  
    if(strlen(argv[1]) >= sizeof(filepath)) {  
        printf("Video path is too long, %d bytes. It should be less than %d bytes.\n",  
            strlen(argv[1]), sizeof(filepath));  
        return -1;  
    }  
    strncpy(filepath, argv[1], sizeof(filepath));  
   
    /* Open the specified file(autodetecting the format) and read the header, exporting the information 
     * stored there into AVFormatContext. The codecs are not opened.  
     * The stream must be closed with avformat_close_input(). */  
    ret = avformat_open_input(&pFormatCtx, filepath, NULL, NULL);  
    if(ret != 0) {  
        printf("[error]avformat_open_input: %s\n", av_err2str(ret));  
        return -1;  
    }  
   
    /* Some formats do not have a header or do not store enough information there, so it it recommended  
         * that you call the avformat_find_stream_info() which tries to read and decode a few frames to find  
         * missing information.  
         * This is useful for file formats with no headers such as MPEG. */  
    ret = avformat_find_stream_info(pFormatCtx, NULL);  
    if(ret < 0) {  
        printf("[error]avformat_find_stream_info: %s\n", av_err2str(ret));  
          
        avformat_close_input(&pFormatCtx);  
        return -1;  
    }  
   
    /* Find out which stream is video, audio, and subtitle. */  
    videoindex = audioindex = titleindex = -1;  
    for(i = 0; i < pFormatCtx->nb_streams; i++) {  
        avStream = pFormatCtx->streams[i];  
        switch(avStream->codecpar->codec_type) {  
            case AVMEDIA_TYPE_VIDEO:  
                videoindex = i;  
                break;  
            case AVMEDIA_TYPE_AUDIO:  
                audioindex = i;  
                break;  
            case AVMEDIA_TYPE_SUBTITLE:  
                titleindex = i;  
                break;  
        }  
    }  
    if(videoindex == -1) {  
        printf("Didn't find a video stream.\n");  
        avformat_close_input(&pFormatCtx);  
        return -1;  
    }  
   
    /* Open video codec */  
    pCodecCtx = avcodec_alloc_context3(NULL);   /* It should be freed with avcodec_free_context() */  
    if(!pCodecCtx) {  
        printf("[error]avcodec_alloc_context3() fail\n");  
        avformat_close_input(&pFormatCtx);  
        return -1;  
    }  
    ret = avcodec_parameters_to_context(pCodecCtx, pFormatCtx->streams[videoindex]->codecpar);  
    if(ret < 0) {  
        printf("[error]avcodec_parameters_to_context: %s\n", av_err2str(ret));  
        avcodec_free_context(&pCodecCtx);  
        avformat_close_input(&pFormatCtx);  
        return -1;  
    }  
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);  
    if(pCodec == NULL) {  
        printf("Video Codec not found.\n");  
        avcodec_free_context(&pCodecCtx);  
        avformat_close_input(&pFormatCtx);        
        return -1;  
    }  
    ret = avcodec_open2(pCodecCtx, pCodec, NULL);  
    if(ret < 0) {  
        printf("[error]avcodec_open2: %s\n", av_err2str(ret));  
        avcodec_free_context(&pCodecCtx);  
        avformat_close_input(&pFormatCtx);        
        return -1;  
    }  
   
    /* Output info */  
    printf("-------------File Information-------------\n");  
    av_dump_format(pFormatCtx, 0, filepath, 0);  
    printf("------------------------------------------\n");  
   
    /* SDL */  
    pFrame    = av_frame_alloc();   /* av_frame_free(&pFrame); */  
    pFrameYUV = av_frame_alloc();  
    out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(  
                    AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));  
    av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,   
                    AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);  
    img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,   
                pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P,   
                SWS_BICUBIC, NULL, NULL, NULL);  
   
#if OUTPUT_YUV420P  
    fp_yuv = fopen("output.yuv", "wb+");  
#endif  
   
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {  
        printf("Could not initialize SDL - %s\n", SDL_GetError());        
      
#if OUTPUT_YUV420P  
        fclose(fp_yuv);  
#endif  
        av_free(out_buffer);  
        av_frame_free(&pFrame);   
        av_frame_free(&pFrameYUV);  
        avcodec_free_context(&pCodecCtx);  
        avformat_close_input(&pFormatCtx);        
        return -1;    
    }  
   
    screen_w = pCodecCtx->width;  
    screen_h = pCodecCtx->height;  
    screen   = SDL_CreateWindow("Simplest ffmpleg player's Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,  
                screen_w, screen_h, SDL_WINDOW_OPENGL);   
    if(!screen) {  
        printf("SDL: could not create window - %s\n", SDL_GetError());  
   
#if OUTPUT_YUV420P  
        fclose(fp_yuv);  
#endif  
        av_free(out_buffer);  
        av_frame_free(&pFrame);   
        av_frame_free(&pFrameYUV);  
        avcodec_free_context(&pCodecCtx);  
        avformat_close_input(&pFormatCtx);        
        return -1;    
    }  
      
    sdlRenderer = SDL_CreateRenderer(screen, -1, 0);  
    sdlTexture  = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,   
                    pCodecCtx->width, pCodecCtx->height);  
    sdlRect.x = 0;  
    sdlRect.y = 0;  
    sdlRect.w = screen_w;  
    sdlRect.h = screen_h;  
    /* SDL End */  
   
    /* For decoding, call avcodec_send_packet() to give the decoder raw compressed data in an AVPacket.  
         *       call avcodec_receive_frame() in a loop until AVERROR_EOF is returned.  
     *       On success, it will return an AVFrame containing uncompressed audio or video data. */  
    packet = (AVPacket *)av_malloc(sizeof(AVPacket));  
    av_init_packet(packet);  
    while(av_read_frame(pFormatCtx, packet) >= 0) {    
        // Only handle video packet  
        if(packet->stream_index != videoindex) {  
            av_packet_unref(packet);  
            av_init_packet(packet);  
            continue;         
        }  
   
        /* @return 0 on success 
         *    AVERROR(EAGAIN): input is not accepted in the current state - user must read output with avcodec_receive_frame() 
         *        AVERROR_EOF: the decoder has been flushed, and no new packets can be sent to it  
         *    AVERROR(EINVAL): codec not opened, it is an encoder, or requires flush  
         *    AVERROR(ENOMEM): failed to add packet to internal queue, or similar */  
        ret = avcodec_send_packet(pCodecCtx, packet);  
        if( ret < 0 )  
            continue;  
      
        /* Got frame */  
        do {  
            /* @return 0 on success, a frame was returned 
             *     AVERROR(EAGAIN): output is not available in this state - user must try to send new input 
             *         AVERROR_EOF: the decoder has been fully flushed, and there will be no more output frames 
             *     AVERROR(EINVAL): codec not opened, or it is an encoder 
             *     other negative values: legitimate decoding errors */  
            ret = avcodec_receive_frame(pCodecCtx, pFrame);  
            if(ret < 0)  
                break;  
            else if(ret == 0) {  /* Got a frame successfully */  
                sws_scale(img_convert_ctx, (const unsigned char * const *)pFrame->data, pFrame->linesize, 0,   
                    pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);  
                          
                /* SDL */  
                SDL_UpdateYUVTexture(sdlTexture, &sdlRect,  
                        pFrameYUV->data[0], pFrameYUV->linesize[0],  
                        pFrameYUV->data[1], pFrameYUV->linesize[1],   
                        pFrameYUV->data[2], pFrameYUV->linesize[2]);  
                SDL_RenderClear(sdlRenderer);  
                SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, &sdlRect);  
                SDL_RenderPresent(sdlRenderer);  
                /* SDL End */  
                SDL_Delay(40);  
            } else if(ret == AVERROR_EOF) {  
                avcodec_flush_buffers(pCodecCtx);  
                break;  
            }  
        } while(ret != AVERROR(EAGAIN));  
   
        av_packet_unref(packet);  
        av_init_packet(packet);  
    }  
   
    sws_freeContext(img_convert_ctx);  
   
#if OUTPUT_YUV420P  
    fclose(fp_yuv);  
#endif  
   
    SDL_Quit();  
      
    av_free(packet);  
    av_free(out_buffer);  
    av_frame_free(&pFrame);   
    av_frame_free(&pFrameYUV);  
    avcodec_close(pCodecCtx);  
    avcodec_free_context(&pCodecCtx);  
    avformat_close_input(&pFormatCtx);  
   
    return 0;  
}  