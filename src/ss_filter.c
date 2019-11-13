#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>

#include <ss_filter.h>
#include <ss_stream.h>

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

int init_filters(streamMap * stream){
    char args[512];
    int ret = 0;
    int type = -1;
    const AVFilter *buffersrc = NULL;
    const AVFilter *buffersink = NULL;
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    avfilter_register_all();
    switch(stream->input_fm->fmt_ctx->streams[0]->codec->codec_type){
        case AVMEDIA_TYPE_VIDEO: 
            type=0;
            //filter_graph_v = avfilter_graph_alloc();有可能被更换内存地址
            //stream->filtermap->filter_graph = avfilter_graph_alloc();
             if(!outputs || !inputs || !stream->filtermap->filter_graph){
                    av_log(NULL,AV_LOG_ERROR,"the inpus/ouputs/filter_graph cannot init.\n");
                    return -1;
             } 
            AVCodecContext *dec_v_main_ctx=stream->codecmap->dec_ctx;
            AVCodecContext *codec_v_main_ctx=stream->codecmap->codec_ctx;
            buffersrc=avfilter_get_by_name("buffer");
            buffersink=avfilter_get_by_name("buffersink");
            if(!buffersrc){
                av_log(NULL,AV_LOG_ERROR,"filtering source element not found\n");
                return -1;
            } 
            if(!buffersink){
                av_log(NULL,AV_LOG_ERROR,"filtering sink element not found\n");
                return -1;
            } 
            snprintf(args,sizeof(args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                                    dec_v_main_ctx->width,dec_v_main_ctx->height,dec_v_main_ctx->pix_fmt,
                                    dec_v_main_ctx->time_base.num,dec_v_main_ctx->time_base.den,
                                    dec_v_main_ctx->sample_aspect_ratio.num,dec_v_main_ctx->sample_aspect_ratio.den);
            av_log(NULL,AV_LOG_DEBUG,"%s\n",args);
            //ret = avfilter_graph_create_filter(&buffersrc_ctx,buffersrc,"in", args,NULL,filter_graph_v);
            ret = avfilter_graph_create_filter(&stream->filtermap->buffersrc_ctx,buffersrc,"in", args,NULL,stream->filtermap->filter_graph);
            if(ret<0){
                av_log(NULL,AV_LOG_ERROR,"cannot create buffer source\n");
                return ret;
            } else av_log(NULL,AV_LOG_DEBUG,"successed create buffer source\n");
            //ret = avfilter_graph_create_filter(&buffersink_ctx,buffersink,"out",
            ret = avfilter_graph_create_filter(&stream->filtermap->buffersink_ctx,buffersink,"out",
                                       NULL,NULL,stream->filtermap->filter_graph);
            if(ret<0){
                av_log(NULL,AV_LOG_ERROR,"cannot create buffer sink\n");
                return ret;
            } else av_log(NULL,AV_LOG_DEBUG,"successed create buffer sink\n");
            //ret = av_opt_set_bin(buffersink_ctx,"pix_fmts",
            ret = av_opt_set_bin(stream->filtermap->buffersink_ctx,"pix_fmts",
                        (uint8_t*)&codec_v_main_ctx->pix_fmt,sizeof(codec_v_main_ctx->pix_fmt),
                        AV_OPT_SEARCH_CHILDREN);
            if(ret<0){
                av_log(NULL,AV_LOG_ERROR,"cannot set output pixel format.\n");
                return ret;
            } 
            break;
        case AVMEDIA_TYPE_AUDIO:
            type = 1;
             AVCodecContext *dec_a_ctx=stream->codecmap->dec_ctx;
            AVCodecContext *codec_a_ctx=stream->codecmap->codec_ctx;
       //    filter_graph_a = avfilter_graph_alloc();
             if(!outputs || !inputs || !stream->filtermap->filter_graph){
                    av_log(NULL,AV_LOG_ERROR,"the inpus/ouputs/filter_graph cannot init.\n");
                    return -1;
             } 
            buffersrc = avfilter_get_by_name("abuffer");
            buffersink= avfilter_get_by_name("abuffersink");
            if(!buffersrc){
                av_log(NULL,AV_LOG_ERROR,"filtering source element not found\n");
                return -1;
            } 
            if(!buffersink){
                av_log(NULL,AV_LOG_ERROR,"filtering sink element not found\n");
                return -1;
            }  
            if(!dec_a_ctx->channel_layout)
                dec_a_ctx->channel_layout = av_get_default_channel_layout(dec_a_ctx->channels);
            snprintf(args,sizeof(args),"time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRIx64,
                    //dec_a_ctx->time_base.num,dec_a_ctx->time_base.den,dec_a_ctx->sample_rate,
                    dec_a_ctx->time_base.num,dec_a_ctx->time_base.den,dec_a_ctx->sample_rate,
                    av_get_sample_fmt_name(dec_a_ctx->sample_fmt),dec_a_ctx->channel_layout);
             av_log(NULL,AV_LOG_DEBUG,"%s\n",args);
            //ret = avfilter_graph_create_filter(&buffersrc_ctx_a,buffersrc,"in",args,NULL,filter_graph_a);
            ret = avfilter_graph_create_filter(&stream->filtermap->buffersrc_ctx,buffersrc,"in", args,NULL,stream->filtermap->filter_graph);
            if(ret<0){
                 av_log(NULL,AV_LOG_ERROR,"cannot create audio buffer source\n");
                return ret;
            }else  av_log(NULL,AV_LOG_DEBUG,"successed create auido filter buffer source\n");
             //ret = avfilter_graph_create_filter(&buffersink_ctx_a,buffersink,"out",NULL,NULL,filter_graph_a);
             ret = avfilter_graph_create_filter(&stream->filtermap->buffersink_ctx,buffersink,"out",
                                       NULL,NULL,stream->filtermap->filter_graph);
           if(ret<0){
                 av_log(NULL,AV_LOG_ERROR,"cannot create audio filter buffer sink\n");
                return ret;
            }else  av_log(NULL,AV_LOG_DEBUG,"successed create auido filter buffer sink\n");

            //ret = av_opt_set_bin(buffersink_ctx_a,"sample_fmts",
            ret = av_opt_set_bin(stream->filtermap->buffersink_ctx,"sample_fmts",
                                (uint8_t*)&codec_a_ctx->sample_fmt,sizeof(codec_a_ctx->sample_fmt),
                                AV_OPT_SEARCH_CHILDREN);
            if(ret<0){
                 av_log(NULL,AV_LOG_ERROR,"cannot set output audio filter sample format\n");
                return ret;
            }
             //ret = av_opt_set_bin(buffersink_ctx_a,"channel_layouts",
             ret = av_opt_set_bin(stream->filtermap->buffersink_ctx,"channel_layouts",
                                (uint8_t*)&codec_a_ctx->channel_layout,sizeof(codec_a_ctx->channel_layout),
                                AV_OPT_SEARCH_CHILDREN);
            if(ret<0){
                 av_log(NULL,AV_LOG_ERROR,"cannot set output audio filter channel layouts \n");
                return ret;
            }
             //ret = av_opt_set_bin(buffersink_ctx_a,"sample_rates",
             ret = av_opt_set_bin(stream->filtermap->buffersink_ctx,"sample_rates",
                                (uint8_t*)&codec_a_ctx->sample_rate,sizeof(codec_a_ctx->sample_rate),
                                AV_OPT_SEARCH_CHILDREN);
            if(ret<0){
                av_log(NULL,AV_LOG_ERROR,"cannot set output audio filter sample rate\n");
                return ret;
            }
            break;
        default:av_log(NULL,AV_LOG_ERROR,"unknow type while init filter\n");return -1;
    }
    outputs->name       = av_strdup("in");
    //outputs->filter_ctx = ((type)? buffersrc_ctx_a:buffersrc_ctx);
    outputs->filter_ctx = stream->filtermap->buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;

    inputs->name        = av_strdup("out");
    //inputs->filter_ctx  = ((type)? buffersink_ctx_a:buffersink_ctx);
    inputs->filter_ctx  = stream->filtermap->buffersink_ctx;
    inputs->pad_idx     = 0;
    inputs->next        = NULL;
    if(!outputs->name || !inputs->name){
         av_log(NULL,AV_LOG_ERROR,"the outputs/inputs name cannot strdup.\n");
        return -1;
    }
    ret = avfilter_graph_parse_ptr(stream->filtermap->filter_graph,stream->filtermap->descr,&inputs,&outputs,NULL);
    if(ret<0){
        av_log(NULL,AV_LOG_ERROR,"error occured when avfilter_graph_parse_ptr\n");
        return ret;
    } 
    ret = avfilter_graph_config(stream->filtermap->filter_graph,NULL);
    if(ret<0){
        av_log(NULL,AV_LOG_ERROR,"error occured when avfilter_graph_config\n");
        return ret;
    }
    if(type)
        //av_buffersink_set_frame_size(buffersink_ctx_a,codec_a_ctx->frame_size);
        av_buffersink_set_frame_size(stream->filtermap->buffersink_ctx,1024);
    return 0;
}


