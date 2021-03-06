#ifndef _SS_CODEC_H_
#define _SS_CODEC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavcodec/avcodec.h>

typedef struct
{
    AVCodec *codec;
    AVCodec *dec;
    AVCodecContext *codec_ctx;
    AVCodecContext *dec_ctx;
    AVDictionary *opts;
}   codecMap;

void init_codecMap(codecMap * cm);
void free_codecMap(codecMap * cm);
int decode(AVCodecContext *decodec, AVPacket pkt, AVFrame *frame);
int encode(AVCodecContext *codec, AVFrame *frame, AVPacket *pkt);

#endif
