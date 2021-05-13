
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss_process_live.h"
#include <time.h>
#include <sys/time.h>

#define MAX_PIECE 32

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
    if(argc!=4){
    	av_log(NULL,AV_LOG_ERROR,"usage: %s <input rtmp URL> <output video rtp://hostname:port> <output audio rtp://hostname:port>.\n",argv[0]);
    	av_log(NULL,AV_LOG_INFO,"note:[rtp://hostname:port?rtcp=?&localrtpport=?&localrtcpport=?]\n",argv[0]);
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
    livep->output_video->filename	= argv[2]; 
    livep->output_audio->filename	= argv[3]; 
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
    //set outputs && encodecs
    if((ret = set_outputs(livep,VIDEO_STREAM_NEW))<0){
        av_log(NULL,AV_LOG_ERROR,"error occred while set outputs.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed set outputs.\n");
    //set decodecs
    if((ret = set_decodec(livep->input_rtmp, livep->video->codecmap, livep->rtmp_index_video))<0){
        av_log(NULL,AV_LOG_ERROR,"error occurred when open video decodec.\n");
        goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"sucessed set decodec: video\n");
 
//    //read data
//    AVFormatContext *ifmt_ctx = livep->input_rtmp->fmt_ctx;
//    AVPacket *pkt = av_packet_alloc();
//    AVStream *in_stream;
//    AVStream *out_stream;
//
//    while(1){
//	if((ret = av_read_frame(ifmt_ctx,pkt))<0){
//	    av_log(NULL,AV_LOG_ERROR,"av_read_frame() useless.\n");
//	    break;
//	} 
//	/*video*/
//	if(pkt->stream_index == livep->rtmp_index_video){
//            av_log(NULL,AV_LOG_INFO,"read video packet index: %d\n", livep->video->cur_index_pkt_in++);
//            av_log(NULL,AV_LOG_INFO,"read 1 video pkt.pts=%"PRId64" pkt.dts=%"PRId64" pkt.duration=%"PRId64" pkt.size=%d\n",pkt->pts,pkt->dts,pkt->duration,pkt->size);
//	    in_stream  = livep->input_rtmp->fmt_ctx->streams[livep->rtmp_index_video];
//            out_stream = livep->output_video->fmt_ctx->streams[0];
//            ret = write_pkt(pkt,in_stream,out_stream,0,livep->output_video,0);
//            if(ret<0){
//                av_log(NULL,AV_LOG_ERROR,"error occured while video write 1 pkt\n");
//                goto end;
//            }
//	}
//	/*audio*/
//	else if(pkt->stream_index == livep->rtmp_index_audio){
//            av_log(NULL,AV_LOG_INFO,"read audio packet index: %d\n", livep->audio->cur_index_pkt_in++);
//            av_log(NULL,AV_LOG_INFO,"read 1 audio pkt.pts=%"PRId64" pkt.dts=%"PRId64" pkt.duration=%"PRId64" pkt.size=%d\n",pkt->pts,pkt->dts,pkt->duration,pkt->size);
//	    in_stream  = livep->input_rtmp->fmt_ctx->streams[livep->rtmp_index_audio];
//            out_stream = livep->output_audio->fmt_ctx->streams[0];
//            ret = write_pkt(pkt,in_stream,out_stream,0,livep->output_audio,0);
//            if(ret<0){
//                av_log(NULL,AV_LOG_ERROR,"error occured while audio write 1 pkt\n");
//                goto end;
//            }
//	}
//
//	if(livep->video->cur_index_pkt_in == 500)    break;
//    }
 
 end:
    avformat_close_input(&livep->input_rtmp->fmt_ctx);
    //free_codecMap(livep->video->codecmap);
    //free_codecMap(livep->audio->codecmap);
    free_LivePro(livep);
    free(livep);
    if (ret < 0 && ret != AVERROR_EOF & ret != AVERROR(EAGAIN)) {
        av_log(NULL,AV_LOG_ERROR, "Error occurred.\n");
        return -1;
    }
    return 0;
}

