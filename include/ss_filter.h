#ifndef _SS_FILTER_H_
#define _SS_FILTER_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>


typedef struct
{
    AVFilterContext *buffersrc_ctx;
    AVFilterContext *buffersink_ctx;
    AVFilterGraph *filter_graph;
    const char descr;
}   filterMap;

#endif
