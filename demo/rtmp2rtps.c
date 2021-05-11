
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

