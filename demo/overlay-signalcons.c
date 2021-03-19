#include <signal.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss_process.h"
#include <time.h>
#include <sys/time.h>

#define MAX_PIECE 32

sig_atomic_t signaled = 0;
sig_atomic_t gifstart = 0;

void sig_hander(int signum){
	signaled = !signaled;
	gifstart = signaled;
}

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
    	//av_log_set_level(AV_LOG_QUIET);

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
	//meeting->video->filtermap->descr	= "movie=logo.png[wm];[in][wm]overlay=((main_w-overlay_w)/2):((main_h-overlay_h)/2)[out]";
	//meeting->video->filtermap->descr	= "movie=springrocket-1.83s.gif[wm];[in][wm]overlay=((main_w-overlay_w)/2):((main_h-overlay_h)/2)[out]";
	meeting->video->filtermap->descr	= "movie=hulahoop-2.73s.gif[wm];[in][wm]overlay=((main_w-overlay_w)/2):((main_h-overlay_h)/2)[out]";
	//meeting->video->filtermap->descr	= "movie=whiterocket-4s.gif[wm];[in][wm]overlay=((main_w-overlay_w)/2):((main_h-overlay_h)/2)[out]";
	//meeting->video->filtermap->descr	= "movie=rainbowstar-1.6s.gif[wm];[in][wm]overlay=((main_w-overlay_w)/2):((main_h-overlay_h)/2)[out]";
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
	//const char * bitrate			= "1000k";
	int ret 				= 0;
	double gif_duration_s			= 2.73;

	/*开启网络流接收通道*/
	av_dict_set(&meeting->video->input_fm->ops,"protocol_whitelist","file,udp,rtp",0);
	av_dict_set(&meeting->video->input_fm->ops,"buffer_size","655360",0);

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
	//if(init_filters(meeting->video)<0){
	//    av_log(NULL,AV_LOG_ERROR,"could not init video filter.\n");
	//    goto end;
	//}else   av_log(NULL,AV_LOG_DEBUG,"successed init filter: video\n");
	
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

	int64_t		overlayed1stFrame_pts  	= 0;
	int		gif_first_pkt_index 	= 30;
	int		gif_increment_pkt_index	= gif_duration_s * sm_v->input_fm->fmt_ctx->streams[0]->r_frame_rate.num;
	int		gif_final_pkt_index = gif_first_pkt_index + gif_increment_pkt_index;
			av_log(NULL,AV_LOG_DEBUG,"\ngif_increment_pkt_index: %d\n", gif_increment_pkt_index);
	/*ready to delay*/
	//int64_t start_time = 0;
	//start_time = av_gettime();
	//AVRational time_base;
	//AVRational time_base_q = {1,AV_TIME_BASE};
	//int64_t pts_time;
	//int64_t now_time;

	//int testindex = 1;
	int gif_index = 0;

	//signal(SIGINT,sig_hander);
	while(1){
	signal(SIGINT,sig_hander);



//	if(sm_v->cur_index_pkt_in == testindex * 30){
//		signaled = 1;
//		gifstart = signaled;
//		}
//
//	if(sm_v->cur_index_pkt_in == testindex * 30 + 150){
//		signaled = 0;
//		gifstart = signaled;
//		testindex += 10;
//		}




	printf("signaled: %d.\n",signaled);
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

			/*rescale ts to codec tb*/
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
			
			/*adjust the frame pts to sync with gif pts*/
			/*must synced with the filter's effeciently time*/
			//if ((sm_v->cur_index_pkt_in - gif_first_pkt_index) % gif_increment_pkt_index == 0 ){
			//	frames[0]->pts = av_frame_get_best_effort_timestamp(frames[0]);
			//	overlayed1stFrame_pts = frames[0]->pts;
			//	av_log(NULL,AV_LOG_DEBUG,">>>>>>>>overlayed1st frame pts : %"PRId64"<<<<<<<<\n",overlayed1stFrame_pts);



			//	if(init_filters(meeting->video)<0){
			//	    av_log(NULL,AV_LOG_ERROR,"could not init video filter.\n");
			//	    goto end;
			//	}else   av_log(NULL,AV_LOG_DEBUG,"successed init filter: video\n");
			//}	
			/*overlay生效时间段*/
			//if(sm_v->cur_index_pkt_in >= 100 && sm_v->cur_index_pkt_in <= 175){
			//if(sm_v->cur_index_pkt_in >= gif_first_pkt_index && sm_v->cur_index_pkt_in <= gif_final_pkt_index){
			//if(sm_v->cur_index_pkt_in >= gif_first_pkt_index && sm_v->cur_index_pkt_in <= 270){
			//if(sm_v->cur_index_pkt_in >= gif_first_pkt_index ){
			//if( sm_v->cur_index_pkt_in <= 100){
			if( signaled ){
				if(gifstart){
					gif_first_pkt_index = sm_v->cur_index_pkt_in;
					gifstart = 0;


					switch (gif_index % 5){
						case 0: {
							meeting->video->filtermap->descr = "movie=hulahoop-2.73s.gif[wm];[in][wm]overlay=((main_w-overlay_w)/2):((main_h-overlay_h)/2)[out]";
							gif_duration_s	= 2.73;
							break;
						}
						case 1: {
							meeting->video->filtermap->descr = "movie=rainbowstar-1.6s.gif[wm];[in][wm]overlay=((main_w-overlay_w)/2):((main_h-overlay_h)/2)[out]";
							gif_duration_s	= 1.6;
							break;
						}
						case 2: {
							meeting->video->filtermap->descr = "movie=springrocket-1.83s.gif[wm];[in][wm]overlay=((main_w-overlay_w)/2):((main_h-overlay_h)/2)[out]";
							gif_duration_s	= 1.83;
							break;
						}
						case 3: {
							meeting->video->filtermap->descr = "movie=whiterocket-4s.gif[wm];[in][wm]overlay=((main_w-overlay_w)/2):((main_h-overlay_h)/2)[out]";
							gif_duration_s	= 4;
							break;
						}
						case 4: {
							meeting->video->filtermap->descr = "movie=circlerocket-2.4s.gif[wm];[in][wm]overlay=((main_w-overlay_w)/2):((main_h-overlay_h)/2)[out]";
							gif_duration_s	= 2.4;
							break;
						}
						default:printf("none.\n");
					}
					gif_increment_pkt_index	= gif_duration_s * sm_v->input_fm->fmt_ctx->streams[0]->r_frame_rate.num;
					gif_index++;
				}
				/*adjust the frame pts to sync with gif pts*/
				/*must synced with the filter's effeciently time*/
				if ((sm_v->cur_index_pkt_in - gif_first_pkt_index) % gif_increment_pkt_index == 0 ){
					frames[0]->pts = av_frame_get_best_effort_timestamp(frames[0]);
					overlayed1stFrame_pts = frames[0]->pts;
					av_log(NULL,AV_LOG_DEBUG,">>>>>>>>overlayed1st frame pts : %"PRId64"<<<<<<<<\n",overlayed1stFrame_pts);

					if(init_filters(meeting->video)<0){
					    av_log(NULL,AV_LOG_ERROR,"could not init video filter.\n");
					    goto end;
					}else   av_log(NULL,AV_LOG_DEBUG,"successed init filter: video\n");
				}	
	
				/*make frames filting*/
				for (int i = 0; i < frame_count; i++) {
					av_log(NULL,AV_LOG_DEBUG,"got 1 frame->pts=%"PRId64" instream_codec  showpts = %lf  frame->nb_samples=%d\n",frames[i]->pts,frames[i]->pts*av_q2d(in_stream->codec->time_base),frames[i]->nb_samples);

					av_log(NULL,AV_LOG_DEBUG,">>>>>>>>start overlay<<<<<<<<\n");
					av_log(NULL,AV_LOG_DEBUG,">>>>>>>>index_pkt_in : %d<<<<<<<<\n",sm_v->cur_index_pkt_in);
					/*refresh frame pts*/
					frames[i]->pts = av_frame_get_best_effort_timestamp(frames[i]);
					frames[i]->pts -= overlayed1stFrame_pts;
					av_log(NULL,AV_LOG_DEBUG,">>>>>>>>new pts : %"PRId64"<<<<<<<<\n",frames[i]->pts);
					
					
				        int filt_frame_count = filting( filt_frames, MAX_PIECE, sm_v->filtermap, frames[i]);
				        av_log(NULL,AV_LOG_DEBUG,"filt_frame_count :%d\n", ( filt_frame_count ));
				        if(filt_frame_count <= 0) {
				                    // ???
            					av_log(NULL,AV_LOG_DEBUG,"filt_frame_count<=0,filter need more frames.\n");
				                break;
				        } 

                			for (int j = 0; j < filt_frame_count; j++) {
						AVStream* out_stream = meeting->output->fmt_ctx->streams[0];
                			    	av_log(NULL,AV_LOG_DEBUG,"got 1 filt_frame->pts=%"PRId64" outstream_codec showpts = %lf filt_frame->nb_samples=%d\n",filt_frames[j]->pts,filt_frames[j]->pts*av_q2d(out_stream->codec->time_base),filt_frames[j]->nb_samples); /*refresh pts*/
					    	//frames[i]->pts = av_frame_get_best_effort_timestamp(frames[i]);
					    	filt_frames[j]->pts = av_frame_get_best_effort_timestamp(filt_frames[j]);
					    	/*make frames encode*/
                			    	int pkt_count = encode( pkts, MAX_PIECE, sm_v->codecmap->codec_ctx, filt_frames[j]);
                			    	//int pkt_count = encode( pkts, MAX_PIECE, sm_v->codecmap->codec_ctx, frames[i]);
                			    	av_log(NULL,AV_LOG_DEBUG,"pkt_count :%d\n", ( pkt_count ));
                			    	if (pkt_count <= 0) {
                			    	    //???
                			    	    break;
                			    	}
                			    	for (int k = 0; k < pkt_count; k++) {
                			    	    	//print_time_sec();
					    		/*delay part*/
					    		//time_base = ifmt_ctx->streams[0]->time_base;
					    		//time_base = out_stream->codec->time_base;
					    		//pts_time = av_rescale_q(pkts[k]->pts,time_base,AV_TIME_BASE_Q);
					    		//now_time = av_gettime() - start_time;
					    		//if (pts_time > now_time)
					    		//	av_usleep(pts_time - now_time);

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
                    			    //print_time_sec();
					    /*delay part*/
					    //time_base = ifmt_ctx->streams[0]->time_base;
					    //time_base = out_stream->codec->time_base;
					    //pts_time = av_rescale_q(pkts[k]->pts,time_base,AV_TIME_BASE_Q);
					    //now_time = av_gettime() - start_time;
					    //if (pts_time > now_time)
					    //	av_usleep(pts_time - now_time);
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

