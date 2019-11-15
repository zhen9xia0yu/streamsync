#ifndef _SS_STREAM_H_
#define _SS_STREAM_H_

#include "ss_file.h"
#include "ss_codec.h"
#include "ss_filter.h"
#include <libavutil/opt.h>

#define VIDEO_CODEC_ID AV_CODEC_ID_H264
#define AUDIO_CODEC_ID AV_CODEC_ID_AAC
#define CODEC_FLAG_GLOBAL_HEADER AV_CODEC_FLAG_GLOBAL_HEADER
//#define AUDIO_CODEC_ID AV_CODEC_ID_PCM_ALAW
#define STREAM_DURATION   10.0
#define STREAM_FRAME_RATE 25 /* 25 images/s */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */

typedef struct
{
    fileMap *input_fm;
    codecMap *codecmap;
    filterMap *filtermap;
    int cur_index_pkt_in;
    int cur_index_pkt_out;
    int cur_index_frame_in;
    int cur_index_frame_out;
    int cur_pts;
}   streamMap;

void init_streamMap(streamMap * sm);
void free_streamMap(streamMap * sm);
void init_packet(AVPacket *packet);
int init_filters(streamMap * stream);
int set_decoder(streamMap * sm,int stream_id);
int set_pts(AVPacket *pkt,AVStream *stream, int pkt_index);
int write_pkt(AVPacket *pkt,AVStream *in_stream,AVStream *out_stream,
                int stream_index,fileMap *fm);
int transcode_filt(AVPacket pkt, AVPacket *new_pkt, 
                AVStream *in_stream,AVStream *out_stream,
                int pkt_index,AVCodecContext *codec,AVCodecContext *decodec, 
                AVFrame *frame,AVFrame *filt_frame,
                AVFilterContext *buffersrc_ctx,AVFilterContext *buffersink_ctx, 
                int type);


#endif
