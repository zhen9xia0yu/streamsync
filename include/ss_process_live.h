#ifndef _SS_PROCESS_H_
#define _SS_PROCESS_H_

#include "ss_stream.h"

typedef struct
{
    fileMap *input_rtmp;
    fileMap *output_video;
    fileMap *output_audio;
    streamMap *video;
    streamMap *audio;
}   LivePro;
void init_LivePro(LivePro * livep);
void free_LivePro(LivePro * livep);
int set_inputs(LivePro * livep);
//int add_stream(meetPro *meeting,codecMap *cm,enum AVCodecID codec_id,const char *bitrate);
//int set_outputs(meetPro *meeting, int trans_video,const char *bitrate);

#endif
