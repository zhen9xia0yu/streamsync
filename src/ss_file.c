#include <ss_file.h>

void init_fileMap(fileMap * fm){
    fm->fmt_ctx = (AVFormatContext *) calloc(1,sizeof(AVFormatContext));
    fm->ofmt = (AVOutputFormat *) calloc(1,sizeof(AVOutputFormat));
}

void free_fileMap(fileMap * fm){
    free(fm->fmt_ctx);
    free(fm->ofmt);
}


