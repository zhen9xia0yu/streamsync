
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss_process.h"

int main(int argc,char **argv){
    int ret,i;
    static meetPro *meeting1;

    if(argc!=4){
        av_log(NULL,AV_LOG_ERROR,"usage: %s <input video file> <input audio file> <output file>\n",argv[0]);
        //goto end;
    }
    av_log_set_level(AV_LOG_DEBUG);
    av_register_all();
    avformat_network_init();
    meeting1 = (meetPro *) calloc(1,sizeof(meetPro));
    init_meetPro(meeting1);

    meeting1->video_main->input_fm->filename=argv[1];
    meeting1->audio_individual->input_fm->filename=argv[2];
    meeting1->output_main->filename=argv[3];

    av_log(NULL,AV_LOG_DEBUG,"%s\n",meeting1->video_main->input_fm->filename);
    av_log(NULL,AV_LOG_DEBUG,"%s\n",meeting1->audio_individual->input_fm->filename);
    av_log(NULL,AV_LOG_DEBUG,"%s\n",meeting1->output_main->filename);
    
    av_dict_set(&meeting1->video_main->input_fm->ops,"protocol_whitelist","file,udp,rtp",0);
    av_dict_set(&meeting1->audio_individual->input_fm->ops,"protocol_whitelist","file,udp,rtp",0);
    if((ret = set_inputs(meeting1))<0){
        av_log(NULL,AV_LOG_ERROR,"error occred while set inputs.\n");
        //goto end;
    }else   av_log(NULL,AV_LOG_DEBUG,"successed set inputs\n");
   
    free_meetPro(meeting1);
    free(meeting1);

    return 0;
}
