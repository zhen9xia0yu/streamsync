#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ss_stream.h>

void init_streamMap(streamMap * sm){

    sm->input_fm = (fileMap *) calloc(1,sizeof(fileMap));
    sm->codecmap = (codecMap *) calloc(1,sizeof(codecMap));
    sm->filtermap = (filterMap *) calloc(1,sizeof(filterMap));

    init_fileMap(sm->input_fm);
    //init_codecMap(sm->codecmap);
    init_filterMap(sm->filtermap);
   
}
void free_streamMap(streamMap * sm){
    av_log(NULL,AV_LOG_DEBUG,"before free_streamMap.\n");
    free_fileMap(sm->input_fm);
    //free_codecMap(sm->codecmap);
    free_filterMap(sm->filtermap);
    av_log(NULL,AV_LOG_DEBUG,"free_streamMap.\n");
}
void init_packet(AVPacket *packet){
    av_init_packet(packet);
    packet->data=NULL;
    packet->size=0;
}
int set_pts(AVPacket *pkt,AVStream *stream, int pkt_index){
    AVRational time_base1=stream->time_base;
    int64_t calc_duration=(double)AV_TIME_BASE/av_q2d(stream->r_frame_rate);
    pkt->pts=(double)(pkt_index*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
    pkt->dts=pkt->pts;
    pkt->duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);
    //fprintf(stdout,"set new pts&dts.\n");
    return 0;
}
int write_pkt(AVPacket *pkt,AVStream *in_stream,AVStream *out_stream,int stream_index,fileMap *fm){
    int ret;
    if(stream_index)    av_packet_rescale_ts(pkt,out_stream->codec->time_base,out_stream->time_base);
    else   av_packet_rescale_ts(pkt,in_stream->time_base,out_stream->time_base);
    pkt->pos = -1;
    pkt->stream_index=stream_index;
    av_log(NULL,AV_LOG_INFO,"write 1 pkt.pts=%d pkt.dts=%d pkt.duration=%d pkt.size=%d\n",pkt->pts,pkt->dts,pkt->duration,pkt->size);
    if (av_interleaved_write_frame(fm->fmt_ctx,pkt) < 0) {
        av_log(NULL,AV_LOG_ERROR, "Error write packet\n");
        return -1;
    } 
    return 0;
}



