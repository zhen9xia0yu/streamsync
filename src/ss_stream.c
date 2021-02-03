#include <ss_stream.h>

void init_streamMap(streamMap * sm){
    sm->input_fm = (fileMap *) calloc(1,sizeof(fileMap));
    sm->codecmap = (codecMap *) calloc(1,sizeof(codecMap));
    sm->filtermap = (filterMap *) calloc(1,sizeof(filterMap));
    init_fileMap(sm->input_fm);
    //init_codecMap(sm->codecmap);
    init_filterMap(sm->filtermap);
}

void free_streamMap(streamMap * sm){
    free_fileMap(sm->input_fm);
    //free_codecMap(sm->codecmap);
    free_filterMap(sm->filtermap);
}

void init_packet(AVPacket *packet){
    av_init_packet(packet);
    packet->data=NULL;
    packet->size=0;
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
            ret = avfilter_graph_create_filter(&stream->filtermap->buffersrc_ctx,buffersrc,"in", args,NULL,stream->filtermap->filter_graph);
            if(ret<0){
                av_log(NULL,AV_LOG_ERROR,"cannot create buffer source\n");
                return ret;
            } else av_log(NULL,AV_LOG_DEBUG,"successed create buffer source\n");
            ret = avfilter_graph_create_filter(&stream->filtermap->buffersink_ctx,buffersink,"out",
                                       NULL,NULL,stream->filtermap->filter_graph);
            if(ret<0){
                av_log(NULL,AV_LOG_ERROR,"cannot create buffer sink\n");
                return ret;
            } else av_log(NULL,AV_LOG_DEBUG,"successed create buffer sink\n");
            ret = av_opt_set_bin(stream->filtermap->buffersink_ctx,"pix_fmts",
                        (uint8_t*)&codec_v_main_ctx->pix_fmt,sizeof(codec_v_main_ctx->pix_fmt),
                        AV_OPT_SEARCH_CHILDREN);
	
            if(ret<0){
                av_log(NULL,AV_LOG_ERROR,"cannot set output pixel format.\n");
                return ret;
            } else av_log(NULL,AV_LOG_DEBUG,"successed set output pixel format.\n");
            break;
        case AVMEDIA_TYPE_AUDIO:
            type = 1;
             AVCodecContext *dec_a_ctx=stream->codecmap->dec_ctx;
            AVCodecContext *codec_a_ctx=stream->codecmap->codec_ctx;
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
                    dec_a_ctx->time_base.num,dec_a_ctx->time_base.den,dec_a_ctx->sample_rate,
                    av_get_sample_fmt_name(dec_a_ctx->sample_fmt),dec_a_ctx->channel_layout);
             av_log(NULL,AV_LOG_DEBUG,"%s\n",args);
            ret = avfilter_graph_create_filter(&stream->filtermap->buffersrc_ctx,buffersrc,"in", args,NULL,stream->filtermap->filter_graph);
            if(ret<0){
                 av_log(NULL,AV_LOG_ERROR,"cannot create audio buffer source\n");
                return ret;
            }else  av_log(NULL,AV_LOG_DEBUG,"successed create auido filter buffer source\n");
             ret = avfilter_graph_create_filter(&stream->filtermap->buffersink_ctx,buffersink,"out",
                                       NULL,NULL,stream->filtermap->filter_graph);
           if(ret<0){
                 av_log(NULL,AV_LOG_ERROR,"cannot create audio filter buffer sink\n");
                return ret;
            }else  av_log(NULL,AV_LOG_DEBUG,"successed create auido filter buffer sink\n");
            ret = av_opt_set_bin(stream->filtermap->buffersink_ctx,"sample_fmts",
                                (uint8_t*)&codec_a_ctx->sample_fmt,sizeof(codec_a_ctx->sample_fmt),
                                AV_OPT_SEARCH_CHILDREN);
            if(ret<0){
                 av_log(NULL,AV_LOG_ERROR,"cannot set output audio filter sample format\n");
                return ret;
            }
             ret = av_opt_set_bin(stream->filtermap->buffersink_ctx,"channel_layouts",
                                (uint8_t*)&codec_a_ctx->channel_layout,sizeof(codec_a_ctx->channel_layout),
                                AV_OPT_SEARCH_CHILDREN);
            if(ret<0){
                 av_log(NULL,AV_LOG_ERROR,"cannot set output audio filter channel layouts \n");
                return ret;
            }
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
    outputs->filter_ctx = stream->filtermap->buffersrc_ctx;
    outputs->pad_idx    = 0;
    outputs->next       = NULL;
    inputs->name        = av_strdup("out");
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
        av_buffersink_set_frame_size(stream->filtermap->buffersink_ctx,1024);
    return 0;
}

int set_decoder(streamMap * sm,int stream_id){
   fileMap *fm=sm->input_fm;
   codecMap *cm=sm->codecmap;
   cm->dec = avcodec_find_decoder(fm->fmt_ctx->streams[stream_id]->codec->codec_id);
   if(!cm->dec){
       av_log(NULL,AV_LOG_ERROR,"could not find decodec for '%s'\n",avcodec_get_name(fm->fmt_ctx->streams[stream_id]->codec->codec_id));
       return -1;
   }
   cm->dec_ctx = avcodec_alloc_context3(cm->dec);
   cm->dec_ctx->thread_count=1;
   avcodec_copy_context(cm->dec_ctx,fm->fmt_ctx->streams[stream_id]->codec);
   if(avcodec_open2(cm->dec_ctx,NULL,NULL)<0){
       av_log(NULL,AV_LOG_ERROR,"could not open the codec '%s'\n",avcodec_get_name(fm->fmt_ctx->streams[stream_id]->codec->codec_id));
       return -1;
    }else   av_log(NULL,AV_LOG_DEBUG,"seccessed open the decoder '%s'\n",avcodec_get_name(fm->fmt_ctx->streams[stream_id]->codec->codec_id));
   return 0;
}

int set_pts(AVPacket *pkt,AVStream *stream, int pkt_index){
    AVRational time_base1=stream->time_base;
    int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(stream->r_frame_rate);
    av_log(NULL,AV_LOG_DEBUG,"av_time_base:%d  \n",AV_TIME_BASE);
    av_log(NULL,AV_LOG_DEBUG,"stream->r_frame_rate.num=%d   den=%d \n",stream->r_frame_rate.num,stream->r_frame_rate.den);
    av_log(NULL,AV_LOG_DEBUG,"av_q2d(stream->r_frame_rate):%lf    calc_duration=%"PRId64" \n",av_q2d(stream->r_frame_rate),calc_duration);
    pkt->pts=(double)(pkt_index*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
    pkt->dts=pkt->pts;
    pkt->duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
    return 0;
}

int write_pkt(AVPacket *pkt,AVStream *in_stream,AVStream *out_stream,int stream_index,fileMap *fm,int TransOrNot){
    int ret;
    if(TransOrNot)    av_packet_rescale_ts(pkt,out_stream->codec->time_base,out_stream->time_base);
    else   av_packet_rescale_ts(pkt,in_stream->time_base,out_stream->time_base);
    pkt->pos = -1;
    pkt->stream_index=stream_index;
    av_log(NULL,AV_LOG_INFO,"write 1 pkt.pts=%"PRId64" pkt.dts=%"PRId64" pkt.duration=%"PRId64" pkt.size=%d\n",pkt->pts,pkt->dts,pkt->duration,pkt->size);
    av_log(NULL,AV_LOG_DEBUG,"final pktpts=%"PRId64" outstream showpts = %lf \n",pkt->pts,pkt->pts*av_q2d(out_stream->time_base));
    if (av_interleaved_write_frame(fm->fmt_ctx,pkt) < 0) {
        av_log(NULL,AV_LOG_ERROR, "Error write packet\n");
        return -1;
    }
    return 0;
}

//int transcode_filt(AVPacket pkt, AVPacket *new_pkt, AVStream *in_stream,AVStream *out_stream,int pkt_index,AVCodecContext *codec,AVCodecContext *decodec, AVFrame *frame,AVFrame *filt_frame,AVFilterContext *buffersrc_ctx,AVFilterContext *buffersink_ctx, int type){
//    int ret;
//    if(pkt.size){
//        av_packet_rescale_ts(&pkt,in_stream->time_base,in_stream->codec->time_base);
//        av_log(NULL,AV_LOG_DEBUG,"in transcode rescale pkt showpts = %lf \n",pkt.pts*av_q2d(in_stream->codec->time_base));
//    }
//    else{
//        av_log(NULL,AV_LOG_DEBUG,"pkt.size=0",pkt.pts*av_q2d(in_stream->codec->time_base));
//        av_log(NULL,AV_LOG_DEBUG," showpts = %d \n",pkt.pts);
//    }
//    av_log(NULL,AV_LOG_DEBUG,"before into transcode_file big while\n");
//    while(1){
//        av_log(NULL,AV_LOG_DEBUG,"before decode while once :: frame=%x \n",frame);
//        ret = decode(decodec,pkt,frame);
//        pkt.size=0;
//        if(!ret){
//            av_log(NULL,AV_LOG_DEBUG,"got frame->pts=%"PRId64"  showpts = %lf  frame->nb_samples=%d",frame->pts,frame->pts*av_q2d(in_stream->codec->time_base),frame->nb_samples);
//            frame->pts = av_frame_get_best_effort_timestamp(frame);
//            av_log(NULL,AV_LOG_DEBUG," after get best pts=%"PRId64" showpts = %lf \n",frame->pts,frame->pts*av_q2d(in_stream->codec->time_base));
//            while(1){
//                av_log(NULL,AV_LOG_DEBUG,"before filt while once\n");
//                ret = filting(frame,filt_frame,buffersrc_ctx,buffersink_ctx);
//                frame=NULL;
//                if( ret == AVERROR_EOF)   return ret;
//                else if(ret == AVERROR(EAGAIN)) break;
//                else if(ret<0){
//                    av_log(NULL,AV_LOG_ERROR,"error occured while filt\n");
//                    return ret;
//                }
//                //av_log(NULL,AV_LOG_DEBUG,"got filtframe->pts=%lf  incodec showpts = %lf ",filt_frame->pts,filt_frame->pts*av_q2d(in_stream->codec->time_base));
//                av_log(NULL,AV_LOG_DEBUG,"got filtframe->pts=%"PRId64" outcodec showpts = %lf filtframe->nb_samples=%d\n",filt_frame->pts,filt_frame->pts*av_q2d(out_stream->codec->time_base),filt_frame->nb_samples);
//
//                ret = encode(codec,filt_frame,new_pkt);
//                if(filt_frame)
//                    av_frame_unref(filt_frame);
//                if(ret == AVERROR_EOF) return ret;
//                else if(ret == AVERROR(EAGAIN)) continue;
//                else if(ret<0){
//                        av_log(NULL,AV_LOG_ERROR,"error occured while encode\n");
//                        return -1;
//                }
//                else if(ret == 0 )  return 0;
//            }
//        }else if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
//            ret = filting(NULL,filt_frame,buffersrc_ctx,buffersink_ctx);
//            if( ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
//                ret = encode(codec,NULL,new_pkt);
//                if( ret == AVERROR_EOF || ret == AVERROR(EAGAIN))   return ret;
//                else if(ret<0){
//                        av_log(NULL,AV_LOG_ERROR,"error occured while encode\n");
//                        return -1;
//                }
//                else if(ret == 0 )  return 0;
//            }
//            else if(ret<0){
//                av_log(NULL,AV_LOG_ERROR,"error occured while filt\n");
//                return ret;
//            }
//            //av_log(NULL,AV_LOG_DEBUG,"got filtframe->pts=%lf  incodec showpts = %lf ",filt_frame->pts,filt_frame->pts*av_q2d(in_stream->codec->time_base));
//            av_log(NULL,AV_LOG_DEBUG,"got filtframe->pts=%"PRId64"  outcodec showpts = %lf filtframe->nb_samples=%d\n",filt_frame->pts,filt_frame->pts*av_q2d(out_stream->codec->time_base),filt_frame->nb_samples);
//
//            ret = encode(codec,filt_frame,new_pkt);
//            if( ret == AVERROR_EOF || ret == AVERROR(EAGAIN))   return ret;
//            else if(ret<0){
//                    av_log(NULL,AV_LOG_ERROR,"error occured while encode\n");
//                    return -1;
//            }
//            else if(ret == 0 )  return 0;
//        }
//        else if(ret<0){
//            av_log(NULL,AV_LOG_ERROR,"error occured while decode\n");
//            return -1;
//        }
//    }
//}
//
//int transcode_unfilt(AVPacket pkt, AVPacket *new_pkt, AVStream *in_stream,AVStream *out_stream,int pkt_index,AVCodecContext *codec,AVCodecContext *decodec, AVFrame *frame, int type){
//    int ret;
//    if(pkt.size){
//        av_packet_rescale_ts(&pkt,in_stream->time_base,in_stream->codec->time_base);
//        av_log(NULL,AV_LOG_DEBUG,"in transcode rescale pkt showpts = %lf \n",pkt.pts*av_q2d(in_stream->codec->time_base));
//    }
//    else{
//        av_log(NULL,AV_LOG_DEBUG,"pkt.size=0",pkt.pts*av_q2d(in_stream->codec->time_base));
//        av_log(NULL,AV_LOG_DEBUG," showpts = %d \n",pkt.pts);
//    }
//    while(1){
//        ret = decode(decodec,pkt,frame);
//        pkt.size=0;
//        if(!ret){
//            av_log(NULL,AV_LOG_DEBUG,"got frame->pts=%lf showpts = %lf ",frame->pts,frame->pts*av_q2d(in_stream->codec->time_base));
//            frame->pts = av_frame_get_best_effort_timestamp(frame);
//            av_log(NULL,AV_LOG_DEBUG,"after get best pts=%lf showpts = %lf \n",frame->pts,frame->pts*av_q2d(in_stream->codec->time_base));
//            switch(frame->pict_type){
//                case AV_PICTURE_TYPE_I:av_log(NULL,AV_LOG_DEBUG,"type:I frame\t\n");break;
//                case AV_PICTURE_TYPE_B:av_log(NULL,AV_LOG_DEBUG,"type:B frame\t\n");break;
//                case AV_PICTURE_TYPE_P:av_log(NULL,AV_LOG_DEBUG,"type:P frame\t\n");break;
//                default:av_log(NULL,AV_LOG_INFO,"type:other\t");break;
//            }
//            ret = encode(codec,frame,new_pkt);
//            if(ret == AVERROR_EOF) return ret;
//            else if(ret == AVERROR(EAGAIN)) continue;
//            else if(ret<0){
//                    av_log(NULL,AV_LOG_ERROR,"error occured while encode\n");
//                    return -1;
//            }
//            else if(ret == 0 )  return 0;
//
//        }else if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
//            av_log(NULL,AV_LOG_DEBUG,"did not get frame from dec\n");
//            ret = encode(codec,NULL,new_pkt);
//            if( ret == AVERROR_EOF || ret == AVERROR(EAGAIN))   return ret;
//            else if(ret<0){
//                    av_log(NULL,AV_LOG_ERROR,"error occured while encode\n");
//                    return -1;
//            }
//            else if(ret == 0 )  return 0;
//        }
//        else if(ret<0){
//            av_log(NULL,AV_LOG_ERROR,"error occured while decode\n");
//            return -1;
//        }
//    }
//}

