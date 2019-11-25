#include <ss_codec.h>

void init_codecMap(codecMap * cm){
    cm->codec = (AVCodec *) calloc(1,sizeof(AVCodec));
    cm->dec = (AVCodec *) calloc(1,sizeof(AVCodec));
    cm->codec_ctx = (AVCodecContext *) calloc(1,sizeof(AVCodecContext));
    cm->dec_ctx = (AVCodecContext *) calloc(1,sizeof(AVCodecContext));
}

void free_codecMap(codecMap * cm){
    avcodec_free_context(&cm->codec_ctx);
    avcodec_free_context(&cm->dec_ctx);
    av_dict_free(&cm->opts);
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

