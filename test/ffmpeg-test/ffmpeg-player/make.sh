#!/bin/sh
export PKG_CONFIG_PATH=/home/quange/ffmpeg_build/lib/pkgconfig/:$PKG_CONFIG_PATH

#echo  `pkg-config "libavcodec" --cflags --libs`
#gcc simplest_ffmpeg_player.c -g -o smp.out -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs`

#gcc simplest_ffmpeg_player_v2.c -g -o smp_v2.out   -lSDL2  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs`

#gcc simplest_ffmpeg_player_su.c -g -o smp111.out -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs`


#gcc simplest_ffmpeg_player_suv2.c -g -o smp_suv2.out   -lSDL2  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs`

#gcc test_player.c -g -o test_player.out  -lSDL2  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs`

gcc ffplayer.c -g -o ffplayer.out  -lSDL2  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs`

#gcc test_sdl.c -g -o test_sdl.out -lSDLmain -lSDL  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs`
