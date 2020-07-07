
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss_process.h"

#define MAX_PIECE 32

int main(int argc,char **argv){
    int ret,apkt_over,trans_video,vfile_over,afile_over;
    meetPro *meeting;
    AVFrame *aframe,*filt_aframe;
    AVPacket vpkt,apkt,newapkt;

//        av_log(NULL,AV_LOG_ERROR,"eagain = %d ,eof = %d\n",AVERROR(EAGAIN),AVERROR_EOF);

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
    //meeting->audio->filtermap->descr="aresample=8000";
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
    // add null test
    aframe = av_frame_alloc();
    filt_aframe = av_frame_alloc();
    //ready to syncing streams
    AVFormatContext *ifmt_ctx;
    AVStream *in_stream, *out_stream;
    streamMap *sm_v_main = meeting->video;
    streamMap *sm_a = meeting->audio;

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
    //start
    //while(1){
//        if(av_compare_ts(sm_v_main->cur_pts, sm_v_main->input_fm->fmt_ctx->streams[0]->time_base,
//                         sm_a->cur_pts, sm_a->input_fm->fmt_ctx->streams[0]->time_base)<=0){
//            ifmt_ctx=sm_v_main->input_fm->fmt_ctx;
//            in_stream=ifmt_ctx->streams[0];
//            out_stream=meeting->output->fmt_ctx->streams[0];
//            if(av_read_frame(ifmt_ctx,&vpkt)>=0){
//                do{
//                    ret = set_pts(&vpkt,in_stream,sm_v_main->cur_index_pkt_in);
//                    if(ret<0){
//                        av_log(NULL,AV_LOG_ERROR,"could not set pts\n");
//                        goto end;
//                    }
//                    sm_v_main->cur_index_pkt_in++;
//                    sm_v_main->cur_pts=vpkt.pts;
//                    av_log(NULL,AV_LOG_INFO,"video: ");
////                    ret = write_pkt(&vpkt,in_stream,out_stream,0,meeting->output,0);
//                    av_packet_unref(&vpkt);
//                    if(ret<0){
//                        av_log(NULL,AV_LOG_ERROR,"error occured while write 1 vpkt\n");
//                        goto end;
//                    }
//                    break;
//                }while(av_read_frame(ifmt_ctx,&vpkt)>=0);
//            }else {
//                vfile_over=1;
//                av_log(NULL,AV_LOG_DEBUG,"the video file is over\n");
//                break;
//            }
//        }else{
            //audio process
            ifmt_ctx = sm_a->input_fm->fmt_ctx;
            while ( !(av_read_frame(ifmt_ctx, &apkt) < 0) ) {
                if (apkt.stream_index!=0) {
                    continue;
                }
                AVStream* in_stream = ifmt_ctx->streams[0];
                av_log(NULL,AV_LOG_DEBUG,"the apkt_index:%d\n",sm_a->cur_index_pkt_in);
                sm_a->cur_index_pkt_in++;
                sm_a->cur_pts = apkt.pts;
                av_packet_rescale_ts( &apkt, in_stream->time_base, in_stream->codec->time_base);
                int frame_count = decode( frames, MAX_PIECE, sm_a->codecmap->dec_ctx, &apkt);
                av_log(NULL,AV_LOG_DEBUG,"frame_count :%d\n", ( frame_count ));
                if (frame_count <= 0) {
                    // add something
                    break;
                }
                for (int i = 0; i < frame_count; i++) {
                    int filt_frame_count = filting( filt_frames, MAX_PIECE, sm_a->filtermap, frames[i]);
                    av_log(NULL,AV_LOG_DEBUG,"filt_frame_count :%d\n", ( filt_frame_count ));
                    if(filt_frame_count <= 0) {
                        // ???
                        break;
                    }
                    for (int j = 0; j < filt_frame_count; j++) {
                        int pkt_count = encode( pkts, MAX_PIECE, sm_a->codecmap->codec_ctx, filt_frames[j]);
                        av_log(NULL,AV_LOG_DEBUG,"pkt_count :%d\n", ( pkt_count ));
                        if (pkt_count <= 0) {
                            //???
                            break;
                        }
                        out_stream = meeting->output->fmt_ctx->streams[1];
                        for (int k = 0; k < pkt_count; k++) {
                            av_log(NULL,AV_LOG_INFO,"audio: ");
                            ret = write_pkt(pkts[k], in_stream,out_stream, 1, meeting->output, 1);
                            // ret = ?
                        }
                    }
                }
            }
//
//        in_stream=ifmt_ctx->streams[0];
//        out_stream=meeting->output->fmt_ctx->streams[1];
//    for(;;){
//        av_free_packet(&newapkt);
//        ret = flush_encoder(sm_a->codecmap->codec_ctx,NULL,&newapkt);
//        if( ret == AVERROR_EOF )    break;
//        if( ret == AVERROR(EAGAIN))   continue;
//        else if(ret<0){
//            av_log(NULL,AV_LOG_ERROR,"error occured while encode\n");
//            goto end;
//        }
//        else if(ret == 0 )  {
//            av_log(NULL,AV_LOG_INFO,"audio: ");
//            ret = write_pkt(&newapkt,in_stream,out_stream,1,meeting->output,1);
//            av_free_packet(&newapkt);
//            if(ret<0){
//                av_log(NULL,AV_LOG_ERROR,"error occured while write 1 apkt\n");
//                goto end;
//            }
//        }
//    }

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
