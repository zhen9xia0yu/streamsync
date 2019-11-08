#ifndef _SS_STREAM_H_
#define _SS_STREAM_H_

#include "ss_file.h"
#include "ss_codec.h"
#include "ss_filter.h"

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

#endif
