
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
    meeting1 = (meetPro *) calloc(1,sizeof(meetPro));
    av_log_set_level(AV_LOG_DEBUG);
    init_meetPro(meeting1);
    meeting1->video_main->input_fm->filename=argv[1];
    av_log(NULL,AV_LOG_DEBUG,"%s\n",meeting1->video_main->input_fm->filename);
    av_register_all();
    avformat_network_init();
    av_log(NULL,AV_LOG_DEBUG,"before free\n");
    free_meetPro(meeting1);
    av_log(NULL,AV_LOG_DEBUG,"after free\n");
    free(meeting1);

    return 0;
}
