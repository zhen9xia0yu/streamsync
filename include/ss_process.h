#ifndef _SS_PROCESS_H_
#define _SS_PROCESS_H_

#include "ss_stream.h"

typedef struct
{
    streamMap *video_main;
    streamMap *video_slide;
    streamMap *audio_individual;
    fileMap *output_main;
    fileMap *output_slide;
    fileMap *output_overlay;
}   meetPro;

#endif
