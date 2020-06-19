
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss_process.h"

int main(int argc,char **argv){
    int ret,i,trans_video,trans_audio,vfile_over,afile_over,vfile_auxi_over;
    meetPro *meeting;
    AVPacket vpkt,apkt,vpkt_auxi,*apkt_auxi;
    if(argc!=6){
        av_log(NULL,AV_LOG_ERROR,"usage: %s <input video main file> <input video auxiliary file> <input audio file> <output main file> <output auxiliary file>\n",argv[0]);
        return -1;
    }
    //init
    av_log_set_level(AV_LOG_DEBUG);
    av_register_all();
    avformat_network_init();
    meeting = (meetPro *) calloc(1,sizeof(meetPro));
    init_meetPro(meeting);
    vfile_over=0;
    afile_over=0;
    vfile_auxi_over=0;
    //setting default values
    meeting->video->input_fm->filename=argv[1];
    meeting->video_auxi->input_fm->filename=argv[2];
    meeting->audio->input_fm->filename=argv[3];
    meeting->output->filename=argv[4];
    meeting->output_auxi->filename=argv[5];

    meeting->video->cur_pts=0;
    meeting->video->cur_index_pkt_in=0;

    meeting->video_auxi->cur_pts=0;
    meeting->video_auxi->cur_index_pkt_in=0;

    meeting->audio->cur_pts=0;
    meeting->audio->cur_index_pkt_in=0;

    av_dict_set(&meeting->video->input_fm->ops,"protocol_whitelist","file,udp,rtp",0);
    av_dict_set(&meeting->video_auxi->input_fm->ops,"protocol_whitelist","file,udp,rtp",0);
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
    init_packet(&vpkt_auxi);
    init_packet(&apkt);
    //ready to syncing streams
    AVFormatContext *ifmt_ctx;
    AVStream *in_stream, *out_stream, *out_stream_auxi;
    streamMap *sm_v_main = meeting->video;
    streamMap *sm_v_auxi = meeting->video_auxi;
    streamMap *sm_a = meeting->audio;

    AVRational time_base_main;
    AVRational time_base_auxi;
    AVRational time_base_audio;

    time_base_main = sm_v_main->input_fm->fmt_ctx->streams[0]->time_base;
    time_base_auxi = sm_v_auxi->input_fm->fmt_ctx->streams[0]->time_base;
    time_base_audio = sm_a->input_fm->fmt_ctx->streams[0]->time_base;

    //start
    for(;;){
        if(av_compare_ts(sm_v_main->cur_pts, time_base_main,
                         sm_v_auxi->cur_pts, time_base_auxi) <=0)
            if(av_compare_ts(sm_v_main->cur_pts, time_base_main,
                             sm_a->cur_pts, time_base_audio) <=0) {
                av_log(NULL, AV_LOG_DEBUG, "sm_v_main\n");

                ifmt_ctx =  sm_v_main->input_fm->fmt_ctx;
                in_stream = ifmt_ctx->streams[0];
                out_stream = meeting->output->fmt_ctx->streams[0];
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
                    vfile_over=1;
                    av_log(NULL,AV_LOG_DEBUG,"the video main file is over\n");
                    break;
                }
            }
            else {
                av_log(NULL, AV_LOG_DEBUG, "sm_a\n");

                ifmt_ctx = sm_a->input_fm->fmt_ctx;
                in_stream=ifmt_ctx->streams[0];
                out_stream=meeting->output->fmt_ctx->streams[1];
                out_stream_auxi=meeting->output_auxi->fmt_ctx->streams[1];
                if(av_read_frame(ifmt_ctx,&apkt)>=0){
                    do{
                        if(apkt.stream_index==0){
                            apkt_auxi = av_packet_clone(&apkt);
                            av_log(NULL,AV_LOG_DEBUG,"the apkt_index:%d\n",sm_a->cur_index_pkt_in);
                            sm_a->cur_index_pkt_in++;
                            sm_a->cur_pts=apkt.pts;
                            av_log(NULL,AV_LOG_INFO,"audio: ");
                            ret = write_pkt(&apkt,in_stream,out_stream,1,meeting->output,1);
                            ret = write_pkt(apkt_auxi,in_stream,out_stream_auxi,1,meeting->output_auxi,1);
                            av_packet_unref(&apkt);
                            av_packet_unref(apkt_auxi);
                            if(ret<0){
                                av_log(NULL,AV_LOG_ERROR,"error occured while write 1 apkt\n");
                                goto end;
                            }
                            break;
                       }
                    }while(av_read_frame(ifmt_ctx,&apkt)>=0);
                }else {
                    afile_over=1;
                    av_log(NULL,AV_LOG_DEBUG,"the audio file is over\n");
                    break;
                }
            }
        else
            if(av_compare_ts(sm_v_auxi->cur_pts, time_base_auxi,
                             sm_a->cur_pts, time_base_audio) <=0) {
                av_log(NULL, AV_LOG_DEBUG, "sm_v_auxi\n");

                ifmt_ctx =  sm_v_auxi->input_fm->fmt_ctx;
                in_stream = ifmt_ctx->streams[0];
                out_stream = meeting->output_auxi->fmt_ctx->streams[0];
                if(av_read_frame(ifmt_ctx,&vpkt_auxi)>=0){
                    do{
                        ret = set_pts(&vpkt_auxi,in_stream,sm_v_auxi->cur_index_pkt_in);
                        if(ret<0){
                            av_log(NULL,AV_LOG_ERROR,"could not set pts\n");
                            goto end;
                        }
                        sm_v_auxi->cur_index_pkt_in++;
                        sm_v_auxi->cur_pts=vpkt_auxi.pts;
                        av_log(NULL,AV_LOG_INFO,"video: ");
                        ret = write_pkt(&vpkt_auxi,in_stream,out_stream,0,meeting->output_auxi,0);
                        av_packet_unref(&vpkt_auxi);
                        if(ret<0){
                            av_log(NULL,AV_LOG_ERROR,"error occured while write 1 vpkt\n");
                            goto end;
                        }
                        break;
                    }while(av_read_frame(ifmt_ctx,&vpkt_auxi)>=0);
                }else {
                    vfile_auxi_over=1;
                    av_log(NULL,AV_LOG_DEBUG,"the video main file is over\n");
                    break;
                }

            }
            else {
                av_log(NULL, AV_LOG_DEBUG, "sm_a\n");

                ifmt_ctx = sm_a->input_fm->fmt_ctx;
                in_stream=ifmt_ctx->streams[0];
                out_stream=meeting->output->fmt_ctx->streams[1];
                out_stream_auxi=meeting->output_auxi->fmt_ctx->streams[1];
                if(av_read_frame(ifmt_ctx,&apkt)>=0){
                    do{
                        if(apkt.stream_index==0){
                            apkt_auxi = av_packet_clone(&apkt);
                            av_log(NULL,AV_LOG_DEBUG,"the apkt_index:%d\n",sm_a->cur_index_pkt_in);
                            sm_a->cur_index_pkt_in++;
                            sm_a->cur_pts=apkt.pts;
                            av_log(NULL,AV_LOG_INFO,"audio: ");
                            ret = write_pkt(&apkt,in_stream,out_stream,1,meeting->output,1);
                            ret = write_pkt(apkt_auxi,in_stream,out_stream_auxi,1,meeting->output_auxi,1);
                            av_packet_unref(&apkt);
                            av_packet_unref(apkt_auxi);
                            if(ret<0){
                                av_log(NULL,AV_LOG_ERROR,"error occured while write 1 apkt\n");
                                goto end;
                            }
                            break;
                       }
                    }while(av_read_frame(ifmt_ctx,&apkt)>=0);
                }else {
                    afile_over=1;
                    av_log(NULL,AV_LOG_DEBUG,"the audio file is over\n");
                    break;
                }

            }
    }

        if(afile_over){
            //finish video main
            ifmt_ctx=sm_v_main->input_fm->fmt_ctx;
            in_stream=ifmt_ctx->streams[0];
            out_stream=meeting->output->fmt_ctx->streams[0];
            if(av_read_frame(ifmt_ctx,&vpkt)>=0){
                av_log(NULL,AV_LOG_DEBUG,"the video main file is not over\n");
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
                }while(av_read_frame(ifmt_ctx,&vpkt)>=0);
            }
            //finish video auxi
            ifmt_ctx=sm_v_auxi->input_fm->fmt_ctx;
            in_stream=ifmt_ctx->streams[0];
            out_stream=meeting->output_auxi->fmt_ctx->streams[0];
            if(av_read_frame(ifmt_ctx,&vpkt_auxi)>=0){
                av_log(NULL,AV_LOG_DEBUG,"the video auxi file is not over\n");
                do{
                    ret = set_pts(&vpkt_auxi,in_stream,sm_v_auxi->cur_index_pkt_in);
                    if(ret<0){
                        av_log(NULL,AV_LOG_ERROR,"could not set pts\n");
                        goto end;
                    }
                    sm_v_auxi->cur_index_pkt_in++;
                    sm_v_auxi->cur_pts=vpkt_auxi.pts;
                    av_log(NULL,AV_LOG_INFO,"video: ");
                    ret = write_pkt(&vpkt_auxi,in_stream,out_stream,0,meeting->output_auxi,0);
                    av_packet_unref(&vpkt_auxi);
                    if(ret<0){
                        av_log(NULL,AV_LOG_ERROR,"error occured while write 1 vpkt\n");
                        goto end;
                    }
                }while(av_read_frame(ifmt_ctx,&vpkt_auxi)>=0);
            }

        }
//    if(vfile_over){
//        ifmt_ctx = sm_a->input_fm->fmt_ctx;
//        in_stream=ifmt_ctx->streams[0];
//        out_stream=meeting->output->fmt_ctx->streams[1];
//        if(av_read_frame(ifmt_ctx,&apkt)>=0){
//            av_log(NULL,AV_LOG_DEBUG,"the audio file is not over\n");
//            do{
//                if(apkt.stream_index==0){
//                    av_log(NULL,AV_LOG_DEBUG,"the apkt_index:%d\n",sm_a->cur_index_pkt_in);
//                    sm_a->cur_index_pkt_in++;
//                    sm_a->cur_pts=apkt.pts;
//                    av_log(NULL,AV_LOG_INFO,"audio: ");
//                    ret = write_pkt(&apkt,in_stream,out_stream,1,meeting->output,1);
//                    av_packet_unref(&apkt);
//                    if(ret<0){
//                        av_log(NULL,AV_LOG_ERROR,"error occured while write 1 apkt\n");
//                        goto end;
//                    }
//               }
//            }while(av_read_frame(ifmt_ctx,&apkt)>=0);
//        }else {
//            av_log(NULL,AV_LOG_DEBUG,"the audio file is over\n");
//        }
//    }
//
    av_write_trailer(meeting->output->fmt_ctx);
    av_write_trailer(meeting->output_auxi->fmt_ctx);
end:
    free_meetPro(meeting);
    free(meeting);
    if (ret < 0 && ret != AVERROR_EOF & ret != AVERROR(EAGAIN)) {
        av_log(NULL,AV_LOG_ERROR, "Error occurred.\n");
        return -1;
    }
    return 0;
}
