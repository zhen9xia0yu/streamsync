#ifndef _SS_CODEC_H_
#define _SS_CODEC_H_

#include <libavcodec/avcodec.h>

#define VIDEO_CODEC_ID AV_CODEC_ID_H264
#define AUDIO_CODEC_ID AV_CODEC_ID_AAC
#define CODEC_FLAG_GLOBAL_HEADER AV_CODEC_FLAG_GLOBAL_HEADER
//#define AUDIO_CODEC_ID AV_CODEC_ID_PCM_ALAW
#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

typedef struct
{
    AVCodec *codec;
    AVCodec *dec;
    AVCodecContext *codec_ctx;
    AVCodecContext *dec_ctx;
}   codecMap;


#endif
