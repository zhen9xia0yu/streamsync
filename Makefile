CC := gcc
CFLAGS := -std=c99 -g -Wall -O
INCLUDE := -I ${FFMPEG_HOME}/include -I./include
LIBPATH := -L${FFMPEG_HOME}/lib
LIB := -lpthread -ldl -lavformat -lavcodec -lavutil -lavfilter -lpostproc -lswscale -lswresample -lx264  -lz -lrt
TARGET := structest 

all : $(TARGET)

structest : ./structest.o
	$(CC) $(CFLAGS) $(LIB) $(LIBPATH) $(INCLUDE) -o $@ $^ 

structest.o: test/structest.c
	$(CC) $(INCLUDE) -c $^   

.PHONY: clean
clean :
	rm -rf structest.o
	rm -rf structest

