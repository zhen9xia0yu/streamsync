#ifndef _SS_FILE_H_
#define _SS_FILE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavformat/avformat.h>

typedef struct{
    const char *filename;
    const char *ops;
    AVFormatContext *fmt_ctx;
    AVOutputFormat *ofmt;
}   fileMap;

void init_fileMap(fileMap * fm);
void free_fileMap(fileMap * fm);

#endif
