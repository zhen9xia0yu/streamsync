
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss_process.h"

#define USE_H264BSF 1
#define USE_AACBSF 1

int main(int argc,char **argv){
    int ret,i,apkt_over,vpkt_over,trans_video;
    meetPro *meeting;
    AVFrame *aframe,*filt_aframe,*vframe;
    AVPacket vpkt,apkt,newapkt,newvpkt;
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
    //av_dict_set(&meeting->video->input_fm->ops,"use_wallclock_as_timestamps","1",0);
    av_dict_set(&meeting->audio->input_fm->ops,"protocol_whitelist","file,udp,rtp",0);
    //av_dict_set(&meeting->audio->input_fm->ops,"use_wallclock_as_timestamps","1",0);
    const char * bitrate="2500k";
   //set input
    if((ret = set_inputs(meeting))<0){
        av_log(NULL,AV_LOG_ERROR,"error occred while set inputs.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed set inputs\n");
    //set output & encoders
    trans_video=1;
    if((ret = set_outputs(meeting,trans_video,bitrate))<0){
        av_log(NULL,AV_LOG_ERROR,"error occred while set outputs.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed set outputs.\n");
   //set decoders
    if((ret = set_decoder(meeting->audio,0))<0){
        av_log(NULL,AV_LOG_ERROR,"error occurred when open audio decodec.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"sucessed set decoder: audio\n");
    if((ret = set_decoder(meeting->video,0))<0){
        av_log(NULL,AV_LOG_ERROR,"error occurred when open video decodec.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"sucessed set decoder: video\n");
    //init filters
    if(init_filters(meeting->audio)<0){
        av_log(NULL,AV_LOG_ERROR,"could not init audio filter.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed init filter: audio\n");
    //init packets & frames & bitstream_filter
    init_packet(&vpkt);
    init_packet(&apkt);
    init_packet(&newapkt);
    init_packet(&newvpkt);
    aframe = av_frame_alloc();
    vframe = av_frame_alloc();
    filt_aframe = av_frame_alloc();
    //ready to delay
    int64_t start_time=0;
    start_time=av_gettime();
    AVRational time_base;
    AVRational time_base_q = {1,AV_TIME_BASE};
    int64_t pts_time;
    int64_t now_time;
#if USE_H264BSF
    AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif
#if USE_AACBSF
    AVBitStreamFilterContext* aacbsfc = av_bitstream_filter_init("aac_adtstoasc");
#endif
    //ready to syncing streams
    AVFormatContext *ifmt_ctx;
    AVStream *in_stream, *out_stream;
    streamMap *sm_v_main = meeting->video;
    streamMap *sm_a = meeting->audio;
    double rt_bitrate;
    //start
    while(1){
        if(av_compare_ts(sm_v_main->cur_pts, sm_v_main->input_fm->fmt_ctx->streams[0]->time_base,
                         sm_a->cur_pts, sm_a->input_fm->fmt_ctx->streams[0]->time_base)<=0){
            ifmt_ctx=sm_v_main->input_fm->fmt_ctx;
            in_stream=ifmt_ctx->streams[0];
            out_stream=meeting->output->fmt_ctx->streams[0];
            if(av_read_frame(ifmt_ctx,&vpkt)>=0){
                do{
                    if(vpkt.stream_index==0){
                        av_log(NULL,AV_LOG_DEBUG,"the vpkt_index:%d\n",sm_v_main->cur_index_pkt_in);
                        vpkt_over=0;
                        ret = set_pts(&vpkt,in_stream,sm_v_main->cur_index_pkt_in);
                        if(ret<0){
                            av_log(NULL,AV_LOG_ERROR,"could not set pts\n");
                            goto end;
                        }
                        sm_v_main->cur_index_pkt_in++;
                        sm_v_main->cur_pts=vpkt.pts;
                        while(1){
                           ret = transcode_unfilt(vpkt,&newvpkt,in_stream,out_stream,
                                            sm_v_main->cur_index_pkt_in,
                                            sm_v_main->codecmap->codec_ctx,
                                            sm_v_main->codecmap->dec_ctx,
                                            vframe,0);
                           if(ret==0){
#if USE_H264BSF
                                av_bitstream_filter_filter(h264bsfc, in_stream->codec,NULL,&newvpkt.data,&newvpkt.size,newvpkt.data,newvpkt.size,0);
#endif
                               //delay part
                               time_base = ifmt_ctx->streams[0]->time_base;
                               pts_time = av_rescale_q(vpkt.dts, time_base, time_base_q);
                               now_time = av_gettime() - start_time;
                               if (pts_time > now_time)
                                   av_usleep(pts_time - now_time);

                                av_log(NULL,AV_LOG_INFO,"video: ");
                                rt_bitrate = (newvpkt.size *8)/av_q2d(sm_v_main->codecmap->dec_ctx->time_base)/1000.0;
                                av_log(NULL,AV_LOG_INFO,"bitrate= %7.1fkbits/s\n",rt_bitrate);
                                ret = write_pkt(&newvpkt,in_stream,out_stream,0,meeting->output,1);
                                av_free_packet(&newvpkt);
                                av_free_packet(&vpkt);
                                vpkt.size=0;
                                if(ret<0){
                                    av_log(NULL,AV_LOG_ERROR,"error occured while write 1 vpkt\n");
                                    goto end;
                                }
                            }else if(ret == AVERROR(EAGAIN)|| ret == AVERROR_EOF){
                                if(ret == AVERROR_EOF)
                                    av_log(NULL,AV_LOG_DEBUG,"its eof\n");
                                if(ret == AVERROR(EAGAIN))
                                    av_log(NULL,AV_LOG_DEBUG,"need more data\n");
                                vpkt_over=1;
                                break;
                            }else if(ret<0){
                                av_log(NULL,AV_LOG_ERROR,"error occured while transcoding v\n");
                                goto end;
                            }
                        }
                        if(vpkt_over)  break;
                    }

                }while(av_read_frame(ifmt_ctx,&vpkt)>=0);
            }else {av_log(NULL,AV_LOG_DEBUG,"the video file is over\n");break;}
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
#if USE_AACBSF
                      av_bitstream_filter_filter(aacbsfc,out_stream->codec, NULL, &newapkt.data,&newapkt.size,newapkt.data,newapkt.size,0);
#endif
                                av_log(NULL,AV_LOG_INFO,"audio: ");
                                rt_bitrate = (newapkt.size *8)/av_q2d(sm_a->codecmap->dec_ctx->time_base)/1000.0;
                                av_log(NULL,AV_LOG_INFO,"bitrate= %7.1fkbits/s\n",rt_bitrate);
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
            }else break;
        }
    }
    av_write_trailer(meeting->output->fmt_ctx);
end:
    free_codecMap(meeting->audio->codecmap);
    free_codecMap(meeting->video->codecmap);
    free_meetPro(meeting);
    free(meeting);
   if (ret < 0 && ret != AVERROR_EOF & ret != AVERROR(EAGAIN)) {
        av_log(NULL,AV_LOG_ERROR, "Error occurred.\n");
        return -1;
    }
    return 0;
}
