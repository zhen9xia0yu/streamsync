
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss_process.h"

#define USE_H264BSF 1

int main(int argc,char **argv){
    int ret,i,apkt_over,trans_video,trans_audio;
    meetPro *meeting;
    AVFrame *aframe,*filt_aframe;
    AVPacket vpkt,apkt,newapkt;
    if(argc!=4){
        av_log(NULL,AV_LOG_ERROR,"usage: %s <input video file> <input audio file> <output file>\n",argv[0]);
        return -1;
    }
    //init
    av_log_set_level(AV_LOG_DEBUG);
    av_register_all();
    avformat_network_init();
    meeting = (meetPro *) calloc(1,sizeof(meetPro));
    init_meetPro(meeting);
    //setting default values
    meeting->video->input_fm->filename=argv[1];
    meeting->audio->input_fm->filename=argv[2];
    meeting->output->filename=argv[3];
    meeting->video->cur_pts=0;
    meeting->video->cur_index_pkt_in=0;
    meeting->audio->cur_pts=0;
    meeting->audio->cur_index_pkt_in=0;
    av_dict_set(&meeting->video->input_fm->ops,"protocol_whitelist","file,udp,rtp",0);
    av_dict_set(&meeting->audio->input_fm->ops,"protocol_whitelist","file,udp,rtp",0);
    const char * bitrate="k";
    //set input
    if((ret = set_inputs(meeting))<0){
        av_log(NULL,AV_LOG_ERROR,"error occred while set inputs.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed set inputs\n");
    //set output & encoders
    trans_video=0;
    trans_audio=0;
    if((ret = set_outputs(meeting,trans_video,trans_audio,bitrate))<0){
        av_log(NULL,AV_LOG_ERROR,"error occred while set outputs.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed set outputs.\n");
    init_packet(&vpkt);
    init_packet(&apkt);
//#if USE_H264BSF
//    AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
//#endif
   //ready to syncing streams
    AVFormatContext *ifmt_ctx;
    AVStream *in_stream, *out_stream;
    streamMap *sm_v_main = meeting->video;
    streamMap *sm_a = meeting->audio;
    //start
    while(1){
        if(av_compare_ts(sm_v_main->cur_pts, sm_v_main->input_fm->fmt_ctx->streams[0]->time_base,
                         sm_a->cur_pts, sm_a->input_fm->fmt_ctx->streams[0]->time_base)<=0){
            ifmt_ctx=sm_v_main->input_fm->fmt_ctx;
            in_stream=ifmt_ctx->streams[0];
            out_stream=meeting->output->fmt_ctx->streams[0];
            if(av_read_frame(ifmt_ctx,&vpkt)>=0){
                do{
                    ret = set_pts(&vpkt,in_stream,sm_v_main->cur_index_pkt_in);
                    if(ret<0){
                        av_log(NULL,AV_LOG_ERROR,"could not set pts\n");
                        goto end;
                    }
                    sm_v_main->cur_index_pkt_in++;
                    sm_v_main->cur_pts=vpkt.pts;
                    av_log(NULL,AV_LOG_INFO,"video: ");
                    ret = write_pkt(&vpkt,in_stream,out_stream,0,meeting->output,0);
                    av_packet_unref(&vpkt);
                    if(ret<0){
                        av_log(NULL,AV_LOG_ERROR,"error occured while write 1 vpkt\n");
                        goto end;
                    }
                    break;
                }while(av_read_frame(ifmt_ctx,&vpkt)>=0);
            }else {
                av_log(NULL,AV_LOG_DEBUG,"the video file is over\n");
                break;
            }
        }else {
            ifmt_ctx = sm_a->input_fm->fmt_ctx;
            in_stream=ifmt_ctx->streams[0];
            out_stream=meeting->output->fmt_ctx->streams[1];
            if(av_read_frame(ifmt_ctx,&apkt)>=0){
                do{
                    if(apkt.stream_index==0){
                        av_log(NULL,AV_LOG_DEBUG,"the apkt_index:%d\n",sm_a->cur_index_pkt_in);
                        apkt_over=0;
                        sm_a->cur_index_pkt_in++;
                        sm_a->cur_pts=apkt.pts;
                        av_log(NULL,AV_LOG_INFO,"audio: ");
                        ret = write_pkt(&apkt,in_stream,out_stream,1,meeting->output,1);
                        av_packet_unref(&apkt);
                        if(ret<0){
                            av_log(NULL,AV_LOG_ERROR,"error occured while write 1 apkt\n");
                            goto end;
                        }
                        break;
                   }
                }while(av_read_frame(ifmt_ctx,&apkt)>=0);
            }else {
                av_log(NULL,AV_LOG_DEBUG,"the video file is over\n");
                break;
            }
        }
    }
    av_write_trailer(meeting->output->fmt_ctx);
end:
    free_meetPro(meeting);
    free(meeting);
   if (ret < 0 && ret != AVERROR_EOF & ret != AVERROR(EAGAIN)) {
        av_log(NULL,AV_LOG_ERROR, "Error occurred.\n");
        return -1;
    }
    return 0;
}
