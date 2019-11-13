#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavcodec/avcodec.h>

#include <ss_codec.h>
#include <ss_process.h>

void init_codecMap(codecMap * cm){
    cm->codec = (AVCodec *) calloc(1,sizeof(AVCodec));
    cm->dec = (AVCodec *) calloc(1,sizeof(AVCodec));
    cm->codec_ctx = (AVCodecContext *) calloc(1,sizeof(AVCodecContext));
    cm->dec_ctx = (AVCodecContext *) calloc(1,sizeof(AVCodecContext));
}
void free_codecMap(codecMap * cm){
//    av_log(NULL,AV_LOG_DEBUG,"before free_codecMap.\n");
//    free(cm->codec);
//    free(cm->dec);
//    free(cm->codec_ctx);
//    free(cm->dec_ctx);
//    av_log(NULL,AV_LOG_DEBUG,"free_codecMap.\n");

    avcodec_free_context(&cm->codec_ctx);
    avcodec_free_context(&cm->dec_ctx);
}
int decode(AVCodecContext *decodec, AVPacket pkt, AVFrame *frame){
    int ret;
    if(pkt.size){
        ret = avcodec_send_packet(decodec,&pkt);
        if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"error while sending a pkt to coder\n");
            return -1;
        }
        else av_log(NULL,AV_LOG_DEBUG,"send 1 pkt to decoder ok\n");
    }
    ret = avcodec_receive_frame(decodec,frame);
    if(ret == AVERROR(EAGAIN)){
        av_log(NULL,AV_LOG_DEBUG,"the decoder need more packet\n");
        return ret;
    }else if(ret == AVERROR_EOF){
        av_log(NULL,AV_LOG_DEBUG,"the decoder is eof\n");
        avcodec_flush_buffers(decodec);
        return ret;
    }
    else if(ret<0){
        av_log(NULL,AV_LOG_ERROR,"error while receving frame from decoder\n");
        return -1;
    }
    av_log(NULL,AV_LOG_DEBUG,"got 1 frame from decoder\n");
    return 0;
}
int encode(AVCodecContext *codec, AVFrame *frame, AVPacket *pkt){
    int ret;
    if(frame){
        ret = avcodec_send_frame(codec,frame);
        if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"error while sending 1 frame to encoder\n");
            return ret;
        }
        else av_log(NULL,AV_LOG_DEBUG,"send 1 frame to encoder ok\n");
    }
    ret = avcodec_receive_packet(codec,pkt);
    if(ret == AVERROR(EAGAIN)){
        av_log(NULL,AV_LOG_DEBUG,"the encoder need more frames\n");
        return ret;
    }else if(ret == AVERROR_EOF){
        av_log(NULL,AV_LOG_DEBUG,"the encoder is eof\n");
        return ret;
    }
    else if(ret <0){
        av_log(NULL,AV_LOG_ERROR,"error while receive pkt from encoder\n");
        return -1;
    }
    av_log(NULL,AV_LOG_DEBUG,"got 1 pkt from encoder\n");
    return 0;
}
int transcode_filt(AVPacket pkt, AVPacket *new_pkt, AVStream *in_stream,AVStream *out_stream,int pkt_index,AVCodecContext *codec,AVCodecContext *decodec, AVFrame *frame,AVFrame *filt_frame,AVFilterContext *buffersrc_ctx,AVFilterContext *buffersink_ctx, int type){
    int ret;
    if(pkt.size)
        av_packet_rescale_ts(&pkt,in_stream->time_base,in_stream->codec->time_base);
    while(1){
        ret = decode(decodec,pkt,frame);
        pkt.size=0; 
        if(!ret){
            frame->pts = av_frame_get_best_effort_timestamp(frame);
            while(1){
                ret = filting(frame,filt_frame,buffersrc_ctx,buffersink_ctx);
                frame=NULL;
                if( ret == AVERROR_EOF)   return ret;
                else if(ret == AVERROR(EAGAIN)) break;
                else if(ret<0){
                    av_log(NULL,AV_LOG_ERROR,"error occured while filt\n");
                    return ret;
                } 
                ret = encode(codec,filt_frame,new_pkt);
                if(filt_frame)
                    av_frame_unref(filt_frame);
                if(ret == AVERROR_EOF) return ret;
                else if(ret == AVERROR(EAGAIN)) continue;
                else if(ret<0){
                        av_log(NULL,AV_LOG_ERROR,"error occured while encode\n");
                        return -1;
                } 
                else if(ret == 0 )  return 0;
            }
        }else if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF){
            ret = filting(NULL,filt_frame,buffersrc_ctx,buffersink_ctx);
            if( ret == AVERROR_EOF || ret == AVERROR(EAGAIN)) {
                ret = encode(codec,NULL,new_pkt);
                if( ret == AVERROR_EOF || ret == AVERROR(EAGAIN))   return ret;
                else if(ret<0){
                        av_log(NULL,AV_LOG_ERROR,"error occured while encode\n");
                        return -1;
                } 
                else if(ret == 0 )  return 0;
            }
            else if(ret<0){
                av_log(NULL,AV_LOG_ERROR,"error occured while filt\n");
                return ret;
            } 
            ret = encode(codec,filt_frame,new_pkt);
            if( ret == AVERROR_EOF || ret == AVERROR(EAGAIN))   return ret;
            else if(ret<0){
                    av_log(NULL,AV_LOG_ERROR,"error occured while encode\n");
                    return -1;
            } 
            else if(ret == 0 )  return 0;
        } 
        else if(ret<0){
            av_log(NULL,AV_LOG_ERROR,"error occured while decode\n");
            return -1;
        } 
    }
}

