
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss_process.h"

int main(int argc,char **argv){
    int ret,i;
    static meetPro *meeting;

    if(argc!=4){
        av_log(NULL,AV_LOG_ERROR,"usage: %s <input video file> <input audio file> <output file>\n",argv[0]);
        //goto end;
    }
    //av_log_set_level(AV_LOG_INFO);
    av_log_set_level(AV_LOG_DEBUG);
    av_register_all();
    avformat_network_init();
    meeting = (meetPro *) calloc(1,sizeof(meetPro));
    init_meetPro(meeting);

    meeting->video_main->input_fm->filename=argv[1];
    meeting->audio_individual->input_fm->filename=argv[2];
    meeting->output_main->filename=argv[3];
    meeting->audio_individual->filtermap->descr="aresample=44100";

    av_log(NULL,AV_LOG_DEBUG,"%s\n",meeting->video_main->input_fm->filename);
    av_log(NULL,AV_LOG_DEBUG,"%s\n",meeting->audio_individual->input_fm->filename);
    av_log(NULL,AV_LOG_DEBUG,"%s\n",meeting->output_main->filename);
    
    //set input
    av_dict_set(&meeting->video_main->input_fm->ops,"protocol_whitelist","file,udp,rtp",0);
    av_dict_set(&meeting->audio_individual->input_fm->ops,"protocol_whitelist","file,udp,rtp",0);
    if((ret = set_inputs(meeting))<0){
        av_log(NULL,AV_LOG_ERROR,"error occred while set inputs.\n");
        //goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed set inputs\n");
    //set output & encoders
   if((ret = set_outputs(meeting))<0){
        av_log(NULL,AV_LOG_ERROR,"error occred while set outputs.\n");
        //goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed set outputs.\n");
   //set decoders
    //if((ret = set_decoder(ifmt_ctx_a,0,&audio_dec,dec_a_ctx))<0){
    //if((ret = set_decoder(meeting1->audio_individual->input_fm->fmt_ctx,0,meeting1->audio_individual->codecmap->dec,meeting1->audio_individual->codecmap->dec))<0){
    //if((ret = set_decoder(meeting1,0))<0){
    if((ret = set_decoder(meeting->audio_individual,0))<0){
        av_log(NULL,AV_LOG_ERROR,"error occurred when open audio decodec.\n");
        //goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"sucessed set decoder: audio\n");

    av_log(NULL,AV_LOG_DEBUG,"dec_a->samplerate=%d\n",meeting->audio_individual->codecmap->dec_ctx->sample_rate);
    av_log(NULL,AV_LOG_DEBUG,"codec_a->samplerate=%d\n",meeting->audio_individual->codecmap->codec_ctx->sample_rate);
    av_log(NULL,AV_LOG_DEBUG,"ifmt->samplerate=%d\n",meeting->audio_individual->input_fm->fmt_ctx->streams[0]->codec->sample_rate);

    //init filters
    
    if(init_filters(meeting->audio_individual)<0){
        av_log(NULL,AV_LOG_ERROR,"could not init audio filter.\n");
        //goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed init filter: audio\n");
    
    


    free_meetPro(meeting);
    free(meeting);

    return 0;
}
