#!/bin/sh
export PKG_CONFIG_PATH=/home/quange/ffmpeg_build/lib/pkgconfig/:$PKG_CONFIG_PATH





gcc streamer.c -g -o streamer.out   -lSDL2  `pkg-config "libavcodec" --cflags --libs` `pkg-config "libavformat" --cflags --libs` `pkg-config "libavutil" --cflags --libs` `pkg-config "libswscale" --cflags --libs`


