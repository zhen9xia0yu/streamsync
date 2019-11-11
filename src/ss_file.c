#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavformat/avformat.h>

#include <ss_file.h>

void init_fileMap(fileMap * fm){
    //fm->filename = (char *) calloc(1,sizeof(char));
    //fm->ops = (char  *) calloc(1,sizeof(char));
    fm->fmt_ctx = (AVFormatContext *) calloc(1,sizeof(AVFormatContext));
    fm->ofmt = (AVOutputFormat *) calloc(1,sizeof(AVOutputFormat));
}
void free_fileMap(fileMap * fm){
    //free(fm->filename);
    //free(fm->ops);
    av_log(NULL,AV_LOG_DEBUG,"before free_fileMap.\n");
    free(fm->fmt_ctx);
    free(fm->ofmt);
    av_log(NULL,AV_LOG_DEBUG,"free_fileMap.\n");
}




