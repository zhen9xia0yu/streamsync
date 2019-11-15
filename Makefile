CC := gcc
CFLAGS := -std=c99 -g -Wall -O
INCLUDE := -I ${FFMPEG_HOME}/include -I./include
LIBPATH := -L${FFMPEG_HOME}/lib
LIB := -lpthread -ldl -lavformat -lavcodec -lavutil -lavfilter -lpostproc -lswscale -lswresample -lx264  -lz -lrt
SRCS := $(wildcard src/*.c test/*.c) 
OBJS := $(patsubst %.c, %.o, $(notdir $(SRCS)))
TARGET := transAudio 

all : $(TARGET)

transAudio : $(OBJS) 
	$(CC) $(CFLAGS) $(LIB) $(LIBPATH) $(INCLUDE) -o $@ $^ 
	
$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDE) -c $^   

PHONY: clean
clean :
	rm -rf $(TARGET) $(OBJS)

