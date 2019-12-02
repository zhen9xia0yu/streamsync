#ifndef _SS_PROCESS_H_
#define _SS_PROCESS_H_

#include "ss_stream.h"

typedef struct
{
    streamMap *video;
    streamMap *audio;
    fileMap *output;
}   meetPro;

void init_meetPro(meetPro * mp);
void free_meetPro(meetPro * mp);
int set_inputs(meetPro * meeting);
int add_stream(meetPro *meeting,codecMap *cm,enum AVCodecID codec_id);
int set_outputs(meetPro *meeting, int trans_video);

#endif
