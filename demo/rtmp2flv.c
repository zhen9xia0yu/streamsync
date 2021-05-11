
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss_process_live.h"
#include <time.h>
#include <sys/time.h>

#define MAX_PIECE 32



int set_outputs_flv(LivePro *livep,fileMap *fm_test){
    int ret;
    avformat_alloc_output_context2(&fm_test->fmt_ctx, NULL, "flv", fm_test->filename);
    if (!fm_test->fmt_ctx) {
        av_log(NULL,AV_LOG_ERROR, "Could not create output context\n");
        return -1;
    }
    int i;
    for (i = 0; i < livep->input_rtmp->fmt_ctx->nb_streams; i++) {
        //if(livep->input_rtmp->fmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
	  AVStream *in_stream = livep->input_rtmp->fmt_ctx->streams[i];
          AVStream *out_stream = avformat_new_stream(fm_test->fmt_ctx, in_stream->codec->codec);
          if (!out_stream) {
              printf( "Failed allocating output stream\n");
              ret = AVERROR_UNKNOWN;
              return -1;
          }
          if (avcodec_parameters_from_context(out_stream->codecpar, in_stream->codec) < 0) {
              printf( "Failed to copy context from input to output stream codec context\n");
              return -1;
          }
          out_stream->codec->codec_tag = 0;//与编码器相关的附加信息
          if (fm_test->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
              out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
        //}
    }
    av_log(NULL,AV_LOG_INFO,"==========Output Information==========\n");
    av_dump_format(fm_test->fmt_ctx, 0, fm_test->filename, 1);
    av_log(NULL,AV_LOG_INFO,"======================================\n");
    //Open output file
    if (!(fm_test->ofmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&fm_test->fmt_ctx->pb, fm_test->filename, AVIO_FLAG_WRITE) < 0) {
            av_log(NULL,AV_LOG_ERROR, "Could not open output file '%s'", fm_test->filename);
            return -1;
        }
    }
    //Write file header
    if (avformat_write_header(fm_test->fmt_ctx, NULL) < 0) {
        av_log(NULL,AV_LOG_ERROR, "could not write the header to output.\n");
        return -1;
    }
    return 0;
}




int print_time_sec(void){
    struct tm *p;
    struct timeval tv;
    gettimeofday( &tv, NULL );
    p = gmtime( &tv.tv_sec );
    av_log(NULL,AV_LOG_INFO,"[%d-", 1900 + p->tm_year);
    av_log(NULL,AV_LOG_INFO,"%d-", 1 + p->tm_mon);
    av_log(NULL,AV_LOG_INFO,"%d ", p->tm_mday);
    av_log(NULL,AV_LOG_INFO,"%d:", 8 + p->tm_hour);
    av_log(NULL,AV_LOG_INFO,"%d:", p->tm_min);
    av_log(NULL,AV_LOG_INFO,"%d ", p->tm_sec);
    av_log(NULL,AV_LOG_INFO,"%d] ",tv.tv_usec );
    return 0;
}
int main( int argc, char **argv){
    if(argc!=3){
    	av_log(NULL,AV_LOG_ERROR,"usage: %s <input rtmp URL> <output file name>.\n",argv[0]);
    	return -1;
    }	
    //init
    av_log_set_level(AV_LOG_DEBUG);
    av_register_all();
    avformat_network_init();
    LivePro *livep;
    livep = (LivePro *) calloc(1,sizeof(LivePro));
    init_LivePro(livep);
    //setting default values
    livep->input_rtmp->filename		= argv[1]; 

    fileMap * fm_test;
    fm_test = (fileMap *) calloc(1,sizeof(fileMap));
    init_fileMap(fm_test);
    fm_test->filename = argv[2];

    livep->video->cur_pts		=0;
    livep->video->cur_index_pkt_in	=0;
    livep->video->cur_index_pkt_out	=0;
    livep->audio->cur_pts		=0;
    livep->audio->cur_index_pkt_in	=0;
    livep->audio->cur_index_pkt_out	=0;
    int ret				=0;
    //set input
    if((ret = set_inputs(livep))<0){
        av_log(NULL,AV_LOG_ERROR,"error occred while set inputs.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed set inputs\n");

    //set flv test outputs
    if((ret = set_outputs_flv(livep,fm_test))<0){
        av_log(NULL,AV_LOG_ERROR,"error occred while set outputs.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed set outputs.\n");


    //read data
    AVFormatContext *ifmt_ctx = livep->input_rtmp->fmt_ctx;
    AVPacket *pkt = av_packet_alloc();
    AVStream *in_stream;
    AVStream *out_stream;
    while(1){
	if((ret = av_read_frame(ifmt_ctx,pkt))<0){
	    av_log(NULL,AV_LOG_ERROR,"av_read_frame() useless.\n");
	    break;
	} 
	if(pkt->stream_index == livep->rtmp_index_video){
            av_log(NULL,AV_LOG_INFO,"read video packet index: %d\n", livep->video->cur_index_pkt_in++);
            av_log(NULL,AV_LOG_INFO,"read 1 video pkt.pts=%"PRId64" pkt.dts=%"PRId64" pkt.duration=%"PRId64" pkt.size=%d\n",pkt->pts,pkt->dts,pkt->duration,pkt->size);
	    in_stream  = livep->input_rtmp->fmt_ctx->streams[livep->rtmp_index_video];
            //out_stream = livep->output_video->fmt_ctx->streams[0];
            out_stream = fm_test->fmt_ctx->streams[livep->rtmp_index_video];
            //ret = write_pkt(pkt,in_stream,out_stream,0,livep->output_video,0);
            ret = write_pkt(pkt,in_stream,out_stream,livep->rtmp_index_video,fm_test,0);
            if(ret<0){
                av_log(NULL,AV_LOG_ERROR,"error occured while video write 1 pkt\n");
                goto end;
            }
	}
	else if(pkt->stream_index == livep->rtmp_index_audio){
            av_log(NULL,AV_LOG_INFO,"read audio packet index: %d\n", livep->audio->cur_index_pkt_in++);
            av_log(NULL,AV_LOG_INFO,"read 1 audio pkt.pts=%"PRId64" pkt.dts=%"PRId64" pkt.duration=%"PRId64" pkt.size=%d\n",pkt->pts,pkt->dts,pkt->duration,pkt->size);
	    in_stream  = livep->input_rtmp->fmt_ctx->streams[livep->rtmp_index_audio];
            //out_stream = livep->output_audio->fmt_ctx->streams[0];
            out_stream = fm_test->fmt_ctx->streams[livep->rtmp_index_audio];
            //ret = write_pkt(pkt,in_stream,out_stream,0,livep->output_audio,0);
            ret = write_pkt(pkt,in_stream,out_stream,livep->rtmp_index_audio,fm_test,0);
            if(ret<0){
                av_log(NULL,AV_LOG_ERROR,"error occured while audio write 1 pkt\n");
                goto end;
            }
	}

    if(livep->video->cur_index_pkt_in == 500)
	break;
    }
 
 end:
    avformat_close_input(&livep->input_rtmp->fmt_ctx);
    //free_codecMap(livep->video->codecmap);
    //free_codecMap(livep->audio->codecmap);
    free_fileMap(fm_test);
    free(fm_test);
    free_LivePro(livep);
    free(livep);
    if (ret < 0 && ret != AVERROR_EOF & ret != AVERROR(EAGAIN)) {
        av_log(NULL,AV_LOG_ERROR, "Error occurred.\n");
        return -1;
    }
    return 0;
}

