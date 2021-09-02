#!/bin/sh
CC=gcc
SRCS=$(wildcard *.c */*.c)
OBJS=$(patsubst %.c, %.o, $(SRCS))
FLAG=-g
#LIB=-lavutil -lavformat -lavcodec -lavutil -lswscale -lswresample -lSDL2

export PKG_CONFIG_PATH=/home/quange/ffmpeg_build/lib/pkgconfig/:$PKG_CONFIG_PATH


LIB=-lSDL2  -I/home/quange/ffmpeg_build/include -L/home/quange/ffmpeg_build/lib -L/usr/local/lib -L/home/quange/ffmpeg_build/lib -lavcodec -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -pthread -lm -lz -lfdk-aac -lm -lmp3lame -lm -lopus -lm -lvorbis -lm -logg -lvorbisenc -lvorbis -lm -logg -lx264 -lpthread -lm -ldl -lx265 -lstdc++ -lm -lrt -ldl -lnuma -lswresample -lavutil -lavformat -lavcodec -lavutil -lswscale -lswresample -lm -lavutil -pthread -lm -lXv -lX11 -lXext
NAME=$(wildcard *.c)
TARGET=ffplayer

$(TARGET):$(OBJS)
	#echo  `pkg-config "libavcodec" --cflags --libs`
	$(CC) $(FLAG) -o $@ $^  $(LIB)
	#$(CC) $(LIB) -o $@ $^ $(FLAG)

%.o:%.c
	$(CC) $(FLAG) -o $@ $^ (LIB)

	#$(CC) $(LIB) -o $@ -c $< -g

clean:
	rm -rf $(TARGET) $(OBJS)
