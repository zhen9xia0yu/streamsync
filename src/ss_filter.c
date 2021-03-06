#include <ss_filter.h>

void init_filterMap(filterMap * fM){
    fM->buffersrc_ctx = (AVFilterContext *) calloc(1,sizeof(AVFilterContext));
    fM->buffersink_ctx = (AVFilterContext *) calloc(1,sizeof(AVFilterContext));
    fM->filter_graph = (AVFilterGraph *) calloc(1,sizeof(AVFilterGraph));
}

void free_filterMap(filterMap * fM){
    free(fM->buffersrc_ctx);
    free(fM->buffersink_ctx);
    free(fM->filter_graph);
}

int filting(AVFrame *frame,AVFrame *filt_frame,AVFilterContext *buffersrc_ctx,AVFilterContext *buffersink_ctx){
    int ret;
    if(frame){
        ret = av_buffersrc_add_frame(buffersrc_ctx,frame);
        if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"error while feeding 1 frame to filter\n");
            return -1;
        }
        else av_log(NULL,AV_LOG_DEBUG,"send 1 frame to filter ok\n");
    }
    ret = av_buffersink_get_frame(buffersink_ctx,filt_frame);
    if(ret == AVERROR(EAGAIN)){
        av_log(NULL,AV_LOG_DEBUG,"the filter need more frame\n");
        return ret;
    }else if(ret == AVERROR_EOF){
        av_log(NULL,AV_LOG_DEBUG,"the filter is eof\n");
        return ret;
    }
    else if(ret <0){
        av_log(NULL,AV_LOG_ERROR,"error while receive frame from filter\n");
        return -1;
    } 
    av_log(NULL,AV_LOG_DEBUG,"got 1 frame from filter\n");
    return 0;
}

