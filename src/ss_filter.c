#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>

#include <ss_filter.h>

void init_filterMap(filterMap * fM){
    fM->buffersrc_ctx = (AVFilterContext *) calloc(1,sizeof(AVFilterContext));
    fM->buffersink_ctx = (AVFilterContext *) calloc(1,sizeof(AVFilterContext));
    fM->filter_graph = (AVFilterGraph *) calloc(1,sizeof(AVFilterGraph));
}
void free_filterMap(filterMap * fM){
    av_log(NULL,AV_LOG_DEBUG,"before free_filterMap.\n");
    free(fM->buffersrc_ctx);
    free(fM->buffersink_ctx);
    free(fM->filter_graph);
    av_log(NULL,AV_LOG_DEBUG,"free_filterMap.\n");
}

