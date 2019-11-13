#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavcodec/avcodec.h>

#include <ss_codec.h>

void init_codecMap(codecMap * cm){
    cm->codec = (AVCodec *) calloc(1,sizeof(AVCodec));
    cm->dec = (AVCodec *) calloc(1,sizeof(AVCodec));
    cm->codec_ctx = (AVCodecContext *) calloc(1,sizeof(AVCodecContext));
    cm->dec_ctx = (AVCodecContext *) calloc(1,sizeof(AVCodecContext));
}
void free_codecMap(codecMap * cm){
//    av_log(NULL,AV_LOG_DEBUG,"before free_codecMap.\n");
//    free(cm->codec);
//    free(cm->dec);
//    free(cm->codec_ctx);
//    free(cm->dec_ctx);
//    av_log(NULL,AV_LOG_DEBUG,"free_codecMap.\n");

    avcodec_free_context(&cm->codec_ctx);
    avcodec_free_context(&cm->dec_ctx);
}

