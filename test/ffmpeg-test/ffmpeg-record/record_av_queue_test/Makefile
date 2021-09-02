#!/bin/sh
CC=gcc
SRCS=$(wildcard *.c */*.c)
OBJS=$(patsubst %.c, %.o, $(SRCS))
FLAG=-g
#LIB=-lavutil -lavformat -lavcodec -lavutil -lswscale -lswresample -lSDL2



LIB=-lSDL2 \
	-I/home/quange/ffmpeg_build/include -L/home/quange/ffmpeg_build/lib -L/usr/local/lib -L/home/quange/ffmpeg_build/lib -lavcodec -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -pthread -lm -lz -lfdk-aac -lm -lmp3lame -lm -lopus -lm -lvorbis -lm -logg -lvorbisenc -lvorbis -lm -logg -lx264 -lpthread -lm -ldl -lx265 -lstdc++ -lm -lrt -ldl -lnuma -lswresample -lm -lavutil -pthread -lm -lXv -lX11 -lXext \
	-I/home/quange/ffmpeg_build/include -L/home/quange/ffmpeg_build/lib -L/usr/local/lib -L/home/quange/ffmpeg_build/lib -lavdevice -lm -lxcb -lXau -lXdmcp -lxcb-shape -lxcb -lXau -lXdmcp -lxcb-xfixes -lxcb-render -lxcb-shape -lxcb -lXau -lXdmcp -lasound -lm -ldl -lpthread -lrt -lSDL2 -Wl,--no-undefined -lm -ldl -lasound -lm -ldl -lpthread -lpulse-simple -lpulse -lsndio -lX11 -lXext -lXcursor -lXinerama -lXi -lXrandr -lXss -lXxf86vm -lwayland-egl -lwayland-client -lwayland-cursor -lxkbcommon -lpthread -lrt -lsndio -lXv -lX11 -lXext -lavfilter -pthread -lm -lfreetype -lz -lpng12 -lz -lm -lswscale -lm -lpostproc -lm -lavformat -lm -lz -lavcodec -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -pthread -lm -lz -lfdk-aac -lm -lmp3lame -lm -lopus -lm -lvorbis -lm -logg -lvorbisenc -lvorbis -lm -logg -lx264 -lpthread -lm -ldl -lx265 -lstdc++ -lm -lrt -ldl -lnuma -lswresample -lm -lavutil -pthread -lm -lXv -lX11 -lXext \
	-I/home/quange/ffmpeg_build/include -L/home/quange/ffmpeg_build/lib -L/usr/local/lib -L/home/quange/ffmpeg_build/lib -lavformat -lm -lz -lavcodec -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -pthread -lm -lz -lfdk-aac -lm -lmp3lame -lm -lopus -lm -lvorbis -lm -logg -lvorbisenc -lvorbis -lm -logg -lx264 -lpthread -lm -ldl -lx265 -lstdc++ -lm -lrt -ldl -lnuma -lswresample -lm -lavutil -pthread -lm -lXv -lX11 -lXext \
	-I/home/quange/ffmpeg_build/include -L/home/quange/ffmpeg_build/lib -L/usr/local/lib -L/home/quange/ffmpeg_build/lib -lavformat -lm -lz -lavcodec -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -lvpx -lm -lpthread -pthread -lm -lz -lfdk-aac -lm -lmp3lame -lm -lopus -lm -lvorbis -lm -logg -lvorbisenc -lvorbis -lm -logg -lx264 -lpthread -lm -ldl -lx265 -lstdc++ -lm -lrt -ldl -lnuma -lswresample -lm -lavutil -pthread -lm -lXv -lX11 -lXext \
	-I/home/quange/ffmpeg_build/include -L/home/quange/ffmpeg_build/lib -lswscale -lm -lavutil -pthread -lm -lXv -lX11 -lXext \
	
	
	
	
	
NAME=$(wildcard *.c)
TARGET=av_record_test

$(TARGET):$(OBJS)
	#echo  `pkg-config "libavcodec" --cflags --libs`
	$(CC) $(FLAG) -o $@ $^  $(LIB)
	#echo "==================="
	#$(CC) $(LIB) -o $@ $^ $(FLAG)

%.o:%.c
	$(CC) $(FLAG) -c -o $@ $^ $(LIB)
	#echo "==================="

	#$(CC) $(LIB) -o $@ -c $< -g

clean:
	rm -rf $(TARGET) $(OBJS)
