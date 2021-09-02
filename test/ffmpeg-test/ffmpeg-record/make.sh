##!/bin/sh
export PKG_CONFIG_PATH=/home/quange/ffmpeg_build/lib/pkgconfig/:$PKG_CONFIG_PATH

#echo  `pkg-config "libavcodec" --cflags --libs`


#gcc record_su.c -g -o record_su.out  -lSDL2  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`


#gcc record.c -g -o record.out  -lSDL2  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`

#gcc aac_to_pcm.c -g -o aac_to_pcm.out  -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`

#gcc ffmpeg_get_pcm_muxter.c -g -o ffmpeg_get_pcm_muxter.out  -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`

#gcc ffmpeg_get_pcm_frame.c -g -o ffmpeg_get_pcm_frame.out  -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`

#gcc ffmpeg_get_pcm_muxter.c -g -o ffmpeg_get_pcm_muxter.out  -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`

#gcc ffmpeg_get_camera_muxter.c -g -o ffmpeg_get_camera_muxter.out  -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`

gcc ffmpeg_get_camera_muxter_su.c -g -o ffmpeg_get_camera_muxter_su.out  -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`

#gcc ffmpeg_record_av_queue.c -g -o ffmpeg_record_av_queue.out  -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`

#gcc ffmpeg_record_audio_video.c -g -o ffmpeg_record_audio_video.out  -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`

#gcc ffmpeg_get_camera_frame.c -g -o ffmpeg_get_camera_frame.out  -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`

#gcc record_my.c -g -o record_my.out  -lSDL2  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`

#gcc muxing_pcm.c -g -o muxing_pcm.out  -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`

#gcc muxing.c -g -o muxing.out  -lSDL2  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`
