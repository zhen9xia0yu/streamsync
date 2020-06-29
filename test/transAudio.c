
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss_process.h"

int main(int argc,char **argv){
    int ret,i,apkt_over,trans_video,vfile_over,afile_over;
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
    meeting->audio->filtermap->descr="aresample=44100";
    meeting->video->cur_pts=0;
    meeting->video->cur_index_pkt_in=0;
    meeting->audio->cur_pts=0;
    meeting->audio->cur_index_pkt_in=0;
    av_dict_set(&meeting->video->input_fm->ops,"protocol_whitelist","file,udp,rtp",0);
    av_dict_set(&meeting->audio->input_fm->ops,"protocol_whitelist","file,udp,rtp",0);
    const char * bitrate="2500k";
    vfile_over=0;
    afile_over=0;
    //set input
    if((ret = set_inputs(meeting))<0){
        av_log(NULL,AV_LOG_ERROR,"error occred while set inputs.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed set inputs\n");
    //set output & encoders
    trans_video=0;
    if((ret = set_outputs(meeting,trans_video,bitrate))<0){
        av_log(NULL,AV_LOG_ERROR,"error occred while set outputs.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed set outputs.\n");
   //set decoders
    if((ret = set_decoder(meeting->audio,0))<0){
        av_log(NULL,AV_LOG_ERROR,"error occurred when open audio decodec.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"sucessed set decoder: audio\n");
    av_log(NULL,AV_LOG_DEBUG,"dec_a->samplerate=%d\n",meeting->audio->codecmap->dec_ctx->sample_rate);
    av_log(NULL,AV_LOG_DEBUG,"codec_a->samplerate=%d\n",meeting->audio->codecmap->codec_ctx->sample_rate);
    av_log(NULL,AV_LOG_DEBUG,"ifmt->samplerate=%d\n",meeting->audio->input_fm->fmt_ctx->streams[0]->codec->sample_rate);
    //init filters
    if(init_filters(meeting->audio)<0){
        av_log(NULL,AV_LOG_ERROR,"could not init audio filter.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed init filter: audio\n");
    //init packets & frames & bitstream_filter
    init_packet(&vpkt);
    init_packet(&apkt);
    init_packet(&newapkt);
    aframe = av_frame_alloc();
    filt_aframe = av_frame_alloc();
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
                vfile_over=1;
                av_log(NULL,AV_LOG_DEBUG,"the video file is over\n");
                break;
            }
        }else{
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
                        while(1){
                            ret = transcode_filt(apkt,&newapkt,in_stream,out_stream,
                                            sm_a->cur_index_pkt_in,
                                            sm_a->codecmap->codec_ctx,sm_a->codecmap->dec_ctx,
                                            aframe,filt_aframe,
                                            sm_a->filtermap->buffersrc_ctx,
                                            sm_a->filtermap->buffersink_ctx,1);
                            if(ret==0){
                                av_log(NULL,AV_LOG_INFO,"audio: ");
                                ret = write_pkt(&newapkt,in_stream,out_stream,1,meeting->output,1);
                                av_free_packet(&newapkt);
                                av_free_packet(&apkt);
                                apkt.size=0;
                                if(ret<0){
                                    av_log(NULL,AV_LOG_ERROR,"error occured while write 1 apkt\n");
                                    goto end;
                                }
                            }else if(ret == AVERROR(EAGAIN)|| ret == AVERROR_EOF){
                                if(ret == AVERROR_EOF)
                                    av_log(NULL,AV_LOG_DEBUG,"its eof\n");
                                if(ret == AVERROR(EAGAIN))
                                    av_log(NULL,AV_LOG_DEBUG,"need more data\n");
                                apkt_over=1;
                                break;
                            }else if(ret<0){
                                av_log(NULL,AV_LOG_ERROR,"error occured while transcoding a.\n");
                                goto end;
                            }
                        }
                        if(apkt_over)  break;
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
        ifmt_ctx=sm_v_main->input_fm->fmt_ctx;
        in_stream=ifmt_ctx->streams[0];
        out_stream=meeting->output->fmt_ctx->streams[0];
        if(av_read_frame(ifmt_ctx,&vpkt)>=0){
            av_log(NULL,AV_LOG_DEBUG,"the video file is not over\n");
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
end:
    free_meetPro(meeting);
    free(meeting);
    free_codecMap(meeting->audio->codecmap);
   if (ret < 0 && ret != AVERROR_EOF & ret != AVERROR(EAGAIN)) {
        av_log(NULL,AV_LOG_ERROR, "Error occurred.\n");
        return -1;
    }
    return 0;
}
