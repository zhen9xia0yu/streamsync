CC := gcc
CFLAGS := -std=c99 -g -Wall -O
INCLUDE := -I ${HOME}/.local/include -I./include
LIBPATH := -L ${HOME}/.local/lib
LIB := -lpthread -ldl -lavformat -lavcodec -lavutil -lavfilter -lpostproc -lswscale -lswresample -lx264  -lz -lrt
SRCSALL := $(wildcard src/*.c test/*.c) 
OBJSALL := $(patsubst %.c, %.o, $(notdir $(SRCSALL)))
SRCS := $(wildcard src/*.c) 
OBJS := $(patsubst %.c, %.o, $(notdir $(SRCS)))
TARGET := transAudio transAll

all : $(TARGET)

transAudio : transAudio.o $(OBJS) 
	$(CC) $(CFLAGS) $(LIB) $(LIBPATH) $(INCLUDE) -o $@ $^ 
	
transAll : transAll.o $(OBJS) 
	$(CC) $(CFLAGS) $(LIB) $(LIBPATH) $(INCLUDE) -o $@ $^ 

$(OBJSALL): $(SRCSALL)
	$(CC) $(CFLAGS) $(INCLUDE) -c $^   

PHONY: clean
clean :
	rm -rf $(TARGET) $(OBJSALL)

