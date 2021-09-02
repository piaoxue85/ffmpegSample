##!/bin/sh
export PKG_CONFIG_PATH=/home/quange/ffmpeg_build/lib/pkgconfig/:$PKG_CONFIG_PATH

#echo  `pkg-config "libavcodec" --cflags --libs`






gcc ffmpeg_get_pcm_muxter_su.c -g -o ffmpeg_get_pcm_muxter_su.out  -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`




#gcc ffmpeg_get_camera_muxter_su.c -g -o ffmpeg_get_camera_muxter_su.out  -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs` `pkg-config "libavdevice" --cflags --libs`
