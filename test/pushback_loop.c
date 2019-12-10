
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss_process.h"

#define USE_H264BSF 1
#define USE_AACBSF 1

int main(int argc,char **argv){
    int ret,i;
    meetPro *meeting;
    AVPacket vpkt;
    if(argc!=3){
        av_log(NULL,AV_LOG_ERROR,"usage: %s <input video file> rtp://?:?\n",argv[0]);
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
    meeting->output->filename=argv[2];
    meeting->video->cur_pts=0;
    meeting->video->cur_index_pkt_in=0;
    //set input:
    while(1){
        meeting->video->input_fm->fmt_ctx=null;
        if ((ret = avformat_open_input(&meeting->video->input_fm->fmt_ctx, meeting->video->input_fm->filename, 0, &meeting->video->input_fm->ops)) < 0) {
            av_log(null,av_log_error,"could not open input video file.\n");
            goto end;
        }
        if ((ret = avformat_find_stream_info(meeting->video->input_fm->fmt_ctx, 0)) < 0) {
            av_log(null,av_log_error, "failed to retrieve input video stream information\n");
            goto end;
        }
        av_log(null,av_log_info,"===========input information==========\n");
        av_dump_format(meeting->video->input_fm->fmt_ctx, 0, meeting->video->input_fm->filename, 0);
        av_log(null,av_log_info,"======================================\n");
        //set output
        avformat_alloc_output_context2(&meeting->output->fmt_ctx, null, "flv", meeting->output->filename);
        if (!meeting->output->fmt_ctx) {
            av_log(null,av_log_error, "could not create output context\n");
            goto end;
        }
        *meeting->output->ofmt = *meeting->output->fmt_ctx->oformat;
        av_log(null,av_log_debug,"ofmt=%x\n",meeting->output->ofmt);
        for (i = 0; i < meeting->video->input_fm->fmt_ctx->nb_streams; i++) {
            if(meeting->video->input_fm->fmt_ctx->streams[i]->codec->codec_type==avmedia_type_video){
                avstream *in_stream = meeting->video->input_fm->fmt_ctx->streams[i];
                avstream *out_stream = avformat_new_stream(meeting->output->fmt_ctx, in_stream->codec->codec);
                if (!out_stream) {
                    printf( "failed allocating output stream\n");
                    ret = averror_unknown;
                    goto end;
                }
                if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
                    printf( "failed to copy context from input to output stream codec context\n");
                    goto end;
                }
                out_stream->codec->codec_tag = 0;//与编码器相关的附加信息
                if (meeting->output->fmt_ctx->oformat->flags & avfmt_globalheader)
                    out_stream->codec->flags |= codec_flag_global_header;
                break;
            }
        }
        av_log(null,av_log_info,"==========output information==========\n");
        av_dump_format(meeting->output->fmt_ctx, 0, meeting->output->filename, 1);
        av_log(null,av_log_info,"======================================\n");
        //open output file
        if (!(meeting->output->ofmt->flags & avfmt_nofile)) {
            if (avio_open(&meeting->output->fmt_ctx->pb, meeting->output->filename, avio_flag_write) < 0) {
                av_log(null,av_log_error, "could not open output file '%s'", meeting->output->filename);
                goto end;
            }
        }
        //write file header
        if (avformat_write_header(meeting->output->fmt_ctx, null) < 0) {
            av_log(null,av_log_error, "could not write the header to output.\n");
            goto end;
        }
       //init packets
        init_packet(&vpkt);
#if use_h264bsf
        avbitstreamfiltercontext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif
        //ready to syncing streams
        avformatcontext *ifmt_ctx;
        avstream *in_stream, *out_stream;
        streammap *sm_v_main = meeting->video;
        //ready to delay
        int64_t start_time=0;
        start_time=av_gettime();
        avrational time_base;
        avrational time_base_q = {1,av_time_base};
        int64_t pts_time;
        int64_t now_time;
        //start
        while(1){
              ifmt_ctx=sm_v_main->input_fm->fmt_ctx;
               in_stream=ifmt_ctx->streams[0];
               out_stream=meeting->output->fmt_ctx->streams[0];
               if(av_read_frame(ifmt_ctx,&vpkt)>=0){
                   do{
                       if(vpkt.stream_index==0){
                           av_log(null,av_log_debug,"the vpkt_index:%d\n",sm_v_main->cur_index_pkt_in);
                        ret = set_pts(&vpkt,in_stream,sm_v_main->cur_index_pkt_in);
                        if(ret<0){
                            av_log(null,av_log_error,"could not set pts\n");
                            goto end;
                        }
                        sm_v_main->cur_index_pkt_in++;
                        sm_v_main->cur_pts=vpkt.pts;
#if use_h264bsf
                        ret = av_bitstream_filter_filter(h264bsfc, in_stream->codec,null,&vpkt.data,&vpkt.size,vpkt.data,vpkt.size,0);
                        if(ret<0)
                        {
                            av_log(null,av_log_error,"av_bitstream_filter_filter error\n");
                            goto end;
                        }
#endif
                       //delay part
                       time_base = ifmt_ctx->streams[0]->time_base;
                       pts_time = av_rescale_q(vpkt.dts, time_base, time_base_q);
                       now_time = av_gettime() - start_time;
        //               if (pts_time > now_time)
        //                   av_usleep(pts_time - now_time);

                        av_log(null,av_log_info,"video: ");
                        ret = write_pkt(&vpkt,in_stream,out_stream,0,meeting->output,0);
                        av_packet_unref(&vpkt);
                        if(ret<0){
                            av_log(null,av_log_error,"error occured while write 1 vpkt\n");
                            goto end;
                        }
                        break;
                       }

                   }while(av_read_frame(ifmt_ctx,&vpkt)>=0);
               }else {av_log(null,av_log_debug,"the video file is over\n");break;}
       }
       av_write_trailer(meeting->output->fmt_ctx);
    }
end:
    free_meetPro(meeting);
    free(meeting);
   if (ret < 0 && ret != AVERROR_EOF & ret != AVERROR(EAGAIN)) {
        av_log(NULL,AV_LOG_ERROR, "Error occurred.\n");
        return -1;
    }
    return 0;
}
