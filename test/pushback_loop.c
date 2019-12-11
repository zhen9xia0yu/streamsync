
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
    while(1){
        //set input:
        meeting->video->input_fm->fmt_ctx=NULL;
        if ((ret = avformat_open_input(&meeting->video->input_fm->fmt_ctx, meeting->video->input_fm->filename, 0, &meeting->video->input_fm->ops)) < 0) {
            av_log(NULL,AV_LOG_ERROR,"Could not open input video file.\n");
            goto end;
        }
        if ((ret = avformat_find_stream_info(meeting->video->input_fm->fmt_ctx, 0)) < 0) {
            av_log(NULL,AV_LOG_ERROR, "Failed to retrieve input video stream information\n");
            goto end;
        }
        av_log(NULL,AV_LOG_INFO,"===========Input Information==========\n");
        av_dump_format(meeting->video->input_fm->fmt_ctx, 0, meeting->video->input_fm->filename, 0);
        av_log(NULL,AV_LOG_INFO,"======================================\n");
        //set output
        avformat_alloc_output_context2(&meeting->output->fmt_ctx, NULL, "flv", meeting->output->filename);
        if (!meeting->output->fmt_ctx) {
            av_log(NULL,AV_LOG_ERROR, "Could not create output context\n");
            goto end;
        }
        *meeting->output->ofmt = *meeting->output->fmt_ctx->oformat;
        av_log(NULL,AV_LOG_DEBUG,"ofmt=%x\n",meeting->output->ofmt);
        for (i = 0; i < meeting->video->input_fm->fmt_ctx->nb_streams; i++) {
            if(meeting->video->input_fm->fmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
                AVStream *in_stream = meeting->video->input_fm->fmt_ctx->streams[i];
                AVStream *out_stream = avformat_new_stream(meeting->output->fmt_ctx, in_stream->codec->codec);
                if (!out_stream) {
                    printf( "Failed allocating output stream\n");
                    ret = AVERROR_UNKNOWN;
                    goto end;
                }
                if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
                    printf( "Failed to copy context from input to output stream codec context\n");
                    goto end;
                }
                out_stream->codec->codec_tag = 0;//与编码器相关的附加信息
                if (meeting->output->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                    out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
                break;
            }
        }
        av_log(NULL,AV_LOG_INFO,"==========Output Information==========\n");
        av_dump_format(meeting->output->fmt_ctx, 0, meeting->output->filename, 1);
        av_log(NULL,AV_LOG_INFO,"======================================\n");
        //Open output file
        if (!(meeting->output->ofmt->flags & AVFMT_NOFILE)) {
            if (avio_open(&meeting->output->fmt_ctx->pb, meeting->output->filename, AVIO_FLAG_WRITE) < 0) {
                av_log(NULL,AV_LOG_ERROR, "Could not open output file '%s'", meeting->output->filename);
                goto end;
            }
        }
        //Write file header
        if (avformat_write_header(meeting->output->fmt_ctx, NULL) < 0) {
            av_log(NULL,AV_LOG_ERROR, "could not write the header to output.\n");
            goto end;
        }
       //init packets
        init_packet(&vpkt);
#if USE_H264BSF
        AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
#endif
        //ready to syncing streams
        AVFormatContext *ifmt_ctx;
        AVStream *in_stream, *out_stream;
        streamMap *sm_v_main = meeting->video;
        //ready to delay
        int64_t start_time=0;
        start_time=av_gettime();
        AVRational time_base;
        AVRational time_base_q = {1,AV_TIME_BASE};
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
                           av_log(NULL,AV_LOG_DEBUG,"the vpkt_index:%d\n",sm_v_main->cur_index_pkt_in);
                        ret = set_pts(&vpkt,in_stream,sm_v_main->cur_index_pkt_in);
                        if(ret<0){
                            av_log(NULL,AV_LOG_ERROR,"could not set pts\n");
                            goto end;
                        }
                        sm_v_main->cur_index_pkt_in++;
                        sm_v_main->cur_pts=vpkt.pts;
#if USE_H264BSF
                        ret = av_bitstream_filter_filter(h264bsfc, in_stream->codec,NULL,&vpkt.data,&vpkt.size,vpkt.data,vpkt.size,0);
                        if(ret<0)
                        {
                            av_log(NULL,AV_LOG_ERROR,"av_bitstream_filter_filter error\n");
                            goto end;
                        }
#endif
                       //delay part
                       time_base = ifmt_ctx->streams[0]->time_base;
                       pts_time = av_rescale_q(vpkt.dts, time_base, time_base_q);
                       now_time = av_gettime() - start_time;
                   //    if (pts_time > now_time)
                   //        av_usleep(pts_time - now_time);

                        av_log(NULL,AV_LOG_INFO,"video: ");
                        ret = write_pkt(&vpkt,in_stream,out_stream,0,meeting->output,0);
                        av_packet_unref(&vpkt);
                        if(ret<0){
                            av_log(NULL,AV_LOG_ERROR,"error occured while write 1 vpkt\n");
                            goto end;
                        }
                        break;
                       }

                   }while(av_read_frame(ifmt_ctx,&vpkt)>=0);
               }else {av_log(NULL,AV_LOG_DEBUG,"the video file is over\n");break;}
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
