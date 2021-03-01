
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss_process.h"
#include <time.h>
#include <sys/time.h>

#define MAX_PIECE 32

int pts_small(const streamMap* a, const streamMap* b) {
        int comp = av_compare_ts(a->cur_pts, a->input_fm->fmt_ctx->streams[0]->time_base,
                                 b->cur_pts, b->input_fm->fmt_ctx->streams[0]->time_base);
        return comp <= 0;
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
    	    av_log(NULL,AV_LOG_ERROR,"usage: %s <input video file> overlaypicdir:./logo.png  <output file>\n",argv[0]);
    	    return -1;
    	}

	/*定义日志输出等级*/
    	av_log_set_level(AV_LOG_DEBUG);

	/*初始化所有组件*/
    	av_register_all();

	/*定义结构体指针，申请内存空间*/
    	meetPro *meeting;
    	meeting = (meetPro *) calloc( 1, sizeof(meetPro));
	/*初始化meetPro内部结构体指针*/
	init_meetPro(meeting);

	/*setting values*/
	meeting->video->input_fm->filename	= argv[1];
	meeting->audio->input_fm->filename	= "";
	//meeting->video->filtermap->descr	= "movie=logo_178x52.png[wm];[in][wm]overlay=5:5[out]";
	//meeting->video->filtermap->descr	= "movie=dog.gif[wm];[in][wm]overlay=5:5[out]";
	//meeting->video->filtermap->descr	= "movie=rocket.gif[wm];[in][wm]overlay=5:5[out]";
	//meeting->video->filtermap->descr	= "movie=rocket.gif[wm];[in][wm]overlay=(man_w-overlay_w)/2:(main_h-overlay_h)/2[out]";
	//meeting->video->filtermap->descr	= "movie=logo_178x52.png[wm];[in][wm]overlay=((main_w-overlay_w)/2):((main_h-overlay_h)/2)[out]";
	//meeting->video->filtermap->descr	= "movie=rocket.gif[wm];[in][wm]overlay=((main_w-overlay_w)/2):((main_h-overlay_h)/2)[out]";
	meeting->video->filtermap->descr	= "movie=minibanana.gif[wm];[in][wm]overlay=((main_w-overlay_w)/2):((main_h-overlay_h)/2)[out]";
	//meeting->video->filtermap->descr	= "boxblur";
	//meeting->video->filtermap->descr	= "hflip";
	//meeting->video->filtermap->descr	= "movie=555.h264[wm];[in][wm]overlay=5:5[out]";
	//meeting->video->filtermap->descr	= "null";
	//meeting->video->filtermap->descr	= "scale=78:24,transpose=cclock";
	meeting->output->filename		= argv[2];
	meeting->video->cur_pts			= 0;
	meeting->video->cur_pts			= 0;
	meeting->video->cur_index_pkt_in	= 0;
	meeting->video->cur_index_pkt_out	= 0;
	const char * bitrate			= "2500k";
	int ret 				= 0;

	/*开启网络流接收通道*/
	av_dict_set(&meeting->video->input_fm->ops,"protocol_whitelist","file,udp,rtp",0);

	/*set input*/
	if((ret = set_inputs(meeting))<0){
	    av_log(NULL,AV_LOG_ERROR,"error occred while set inputs.\n");
	    goto end;
	}else   av_log(NULL,AV_LOG_DEBUG,"successed set inputs\n");

	/*set decoders*/
	if((ret = set_decoder(meeting->video,0))<0){
	    av_log(NULL,AV_LOG_ERROR,"error occurred when open video decodec.\n");
	    goto end;
	}else   av_log(NULL,AV_LOG_DEBUG,"sucessed set decoder: video\n");


	/*set output & encoders*/
	int trans_video = 1;
	if((ret = set_outputs(meeting,trans_video,bitrate))<0){
	    av_log(NULL,AV_LOG_ERROR,"error occred while set outputs.\n");
	    goto end;
	}else   av_log(NULL,AV_LOG_DEBUG,"successed set outputs.\n");

	/*init filters*/
	if(init_filters(meeting->video)<0){
	    av_log(NULL,AV_LOG_ERROR,"could not init video filter.\n");
	    goto end;
	}else   av_log(NULL,AV_LOG_DEBUG,"successed init filter: video\n");
	
	/*prepare media data horizen*/
	AVFrame* frames[MAX_PIECE] = {NULL};
	for (int i = 0; i < MAX_PIECE; i++) {
	    frames[i] = av_frame_alloc();
	}
	AVFrame* filt_frames[MAX_PIECE] = {NULL};
	for (int i = 0; i < MAX_PIECE; i++) {
	    filt_frames[i] = av_frame_alloc();
	}
	AVPacket* pkts[MAX_PIECE] = {NULL};
	for (int i = 0; i < MAX_PIECE; i++) {
	    pkts[i] = av_packet_alloc();
	}

	/*ready to overlay*/
	streamMap 	*sm_v     = meeting->video;
	AVFormatContext *ifmt_ctx = sm_v->input_fm->fmt_ctx;
	AVPacket 	*pkt      = av_packet_alloc();

	while(1){
		av_packet_unref(pkt);
		if((ret = av_read_frame(ifmt_ctx, pkt))<0)	break;
		else{
			if(pkt->stream_index != 0)	continue;

			/*record input index & pts*/
			print_time_sec();
			av_log(NULL,AV_LOG_DEBUG,"\nread video packet index: %d\n", sm_v->cur_index_pkt_in);
            		av_log(NULL,AV_LOG_INFO,"read 1 video pkt.pts=%"PRId64" pkt.dts=%"PRId64" pkt.duration=%"PRId64" pkt.size=%d\n",pkt->pts,pkt->dts,pkt->duration,pkt->size);
            		AVStream* in_stream = ifmt_ctx->streams[0];

			/*set h264 pkt pts*/
			if(pkt->pts == AV_NOPTS_VALUE){
            			ret = set_pts(pkt,in_stream,sm_v->cur_index_pkt_in);
				if(ret<0){
				   av_log(NULL,AV_LOG_ERROR,"could not set pts\n");
				   goto end;
				}
				av_log(NULL,AV_LOG_INFO,"after set pts, video pkt.pts=%"PRId64" pkt.dts=%"PRId64" pkt.duration=%"PRId64" pkt.size=%d\n",pkt->pts,pkt->dts,pkt->duration,pkt->size);
			}
            		sm_v->cur_index_pkt_in++;
            		sm_v->cur_pts = pkt->pts;

    av_log(NULL,AV_LOG_DEBUG,"in_stream_codec->time_base: %d/%d  \n",in_stream->codec->time_base.num,in_stream->codec->time_base.den);

            		av_packet_rescale_ts( pkt, in_stream->time_base, in_stream->codec->time_base);
            		av_log(NULL,AV_LOG_INFO,"after rescale, then 1 video pkt.pts=%"PRId64" pkt.dts=%"PRId64" pkt.duration=%"PRId64" pkt.size=%d\n",pkt->pts,pkt->dts,pkt->duration,pkt->size);
			/*got franmes from decodec*/
            		int frame_count = decode( frames, MAX_PIECE, sm_v->codecmap->dec_ctx, pkt);
            		av_log(NULL,AV_LOG_DEBUG,"frame_count :%d\n", ( frame_count ));
            		if (frame_count <= 0) {
            		    // add something
            			av_log(NULL,AV_LOG_DEBUG,"frame_count<=0,decodec need more packets.\n");
            		    	continue;
            		}

			if(sm_v->cur_index_pkt_in >= 100 && sm_v->cur_index_pkt_in <= 200){
				/*make frames filting*/
				for (int i = 0; i < frame_count; i++) {
					av_log(NULL,AV_LOG_DEBUG,"got 1 frame->pts=%"PRId64" instream_codec  showpts = %lf  frame->nb_samples=%d\n",frames[i]->pts,frames[i]->pts*av_q2d(in_stream->codec->time_base),frames[i]->nb_samples);

					/*refresh frame pts*/
					frames[i]->pts = av_frame_get_best_effort_timestamp(frames[i]);
					
				        int filt_frame_count = filting( filt_frames, MAX_PIECE, sm_v->filtermap, frames[i]);
				        av_log(NULL,AV_LOG_DEBUG,"filt_frame_count :%d\n", ( filt_frame_count ));
				        if(filt_frame_count <= 0) {
				                    // ???
            					av_log(NULL,AV_LOG_DEBUG,"filt_frame_count<=0,filter need more frames.\n");
				                break;
				        } 

                			for (int j = 0; j < filt_frame_count; j++) {
						AVStream* out_stream = meeting->output->fmt_ctx->streams[0];
                			    	av_log(NULL,AV_LOG_DEBUG,"got 1 filt_frame->pts=%"PRId64" outstream_codec showpts = %lf filt_frame->nb_samples=%d\n",filt_frames[j]->pts,filt_frames[j]->pts*av_q2d(out_stream->codec->time_base),filt_frames[j]->nb_samples);
					    	/*refresh pts*/
					    	//frames[i]->pts = av_frame_get_best_effort_timestamp(frames[i]);
					    	//filt_frames[j]->pts = av_frame_get_best_effort_timestamp(filt_frames[j]);
					    	/*make frames encode*/
                			    	int pkt_count = encode( pkts, MAX_PIECE, sm_v->codecmap->codec_ctx, filt_frames[j]);
                			    	//int pkt_count = encode( pkts, MAX_PIECE, sm_v->codecmap->codec_ctx, frames[i]);
                			    	av_log(NULL,AV_LOG_DEBUG,"pkt_count :%d\n", ( pkt_count ));
                			    	if (pkt_count <= 0) {
                			    	    //???
                			    	    break;
                			    	}
                			    	for (int k = 0; k < pkt_count; k++) {
                			    	    print_time_sec();
                			    	    av_log(NULL,AV_LOG_INFO,"video: the output packet index: %d ", meeting->video->cur_index_pkt_out);
                			    	    meeting->video->cur_index_pkt_out++;
                			    	    ret = write_pkt(pkts[k], in_stream,out_stream, 0, meeting->output, 1);
                			    	    // ret = ?
                			    	}
                			}
				}
			}
			else{
				/*make frames encode*/
				for (int i = 0; i < frame_count; i++) {
					av_log(NULL,AV_LOG_DEBUG,"got 1 frame->pts=%"PRId64" instream_codec  showpts = %lf  frame->nb_samples=%d\n",frames[i]->pts,frames[i]->pts*av_q2d(in_stream->codec->time_base),frames[i]->nb_samples);
                   			AVStream* out_stream = meeting->output->fmt_ctx->streams[0];
					/*refresh pts*/
					frames[i]->pts = av_frame_get_best_effort_timestamp(frames[i]);
                    			int pkt_count = encode( pkts, MAX_PIECE, sm_v->codecmap->codec_ctx, frames[i]);
                    			av_log(NULL,AV_LOG_DEBUG,"pkt_count :%d\n", ( pkt_count ));
                    			if (pkt_count <= 0) {
                    			    //???
                    			    break;
                    			}
                    			for (int k = 0; k < pkt_count; k++) {
                    			    print_time_sec();
                    			    av_log(NULL,AV_LOG_INFO,"video: the output packet index: %d ", meeting->video->cur_index_pkt_out);
                    			    meeting->video->cur_index_pkt_out++;
                    			    ret = write_pkt(pkts[k], in_stream,out_stream, 0, meeting->output, 1);
                    			    // ret = ?
                    			}
				}	
			}
		}
	}

	av_write_trailer(meeting->output->fmt_ctx);

	/*free*/
    	av_packet_free(&pkt);
    	for (int i = 0; i < MAX_PIECE; i++) {
    	    av_frame_free(&frames[i]);
    	    av_frame_free(&filt_frames[i]);
    	    av_packet_free(&pkts[i]);
    	}


end:
	avformat_close_input(&meeting->video->input_fm->fmt_ctx);
	free_codecMap(meeting->video->codecmap);
	free_meetPro(meeting);
	free(meeting);
	if (ret < 0 && ret != AVERROR_EOF & ret != AVERROR(EAGAIN)) {
	     av_log(NULL,AV_LOG_ERROR, "Error occurred.\n");
	     return -1;
	}
	return 0;
}

