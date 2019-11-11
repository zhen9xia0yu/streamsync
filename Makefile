CC := gcc
CFLAGS := -std=c99 -g -Wall -O
INCLUDE := -I ${FFMPEG_HOME}/include -I./include
LIBPATH := -L${FFMPEG_HOME}/lib
LIB := -lpthread -ldl -lavformat -lavcodec -lavutil -lavfilter -lpostproc -lswscale -lswresample -lx264  -lz -lrt
SRCS := $(wildcard src/*.c test/*.c) 
OBJS := $(patsubst %.c, %.o, $(notdir $(SRCS)))
TARGET := structest 

all : $(TARGET)

structest : $(OBJS) 
	$(CC) $(CFLAGS) $(LIB) $(LIBPATH) $(INCLUDE) -o $@ $^ 

structest.o: test/structest.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $^   
	
ss_file.o: src/ss_file.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $^   

ss_filter.o: src/ss_filter.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $^   

ss_codec.o: src/ss_codec.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $^   

ss_stream.o: src/ss_stream.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $^   

ss_process.o: src/ss_process.c
	$(CC) $(CFLAGS) $(INCLUDE) -c $^   


PHONY: clean
clean :
	rm -rf $(TARGET) $(OBJS)

