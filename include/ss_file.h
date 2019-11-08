#ifndef _SS_FILE_H_
#define _SS_FILE_H_

#include <libavformat/avformat.h>

typedef struct{
    const char *filename;
    const char *ops;
    AVFormatContext *fmt_ctx;
    AVOutputFormat *ofmt;
}   fileMap;

#endif
