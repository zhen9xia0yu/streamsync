#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ss_process.h>

void init_meetPro(meetPro * mp){
    /*
     * 初始化结构体指针，从外到内。
    */

    mp->video_main = (streamMap *) calloc(1,sizeof(streamMap));
    mp->video_slide = (streamMap *) calloc(1,sizeof(streamMap));
    mp->audio_individual = (streamMap *) calloc(1,sizeof(streamMap));
    mp->output_main = (fileMap *) calloc(1,sizeof(fileMap));
    mp->output_slide = (fileMap *) calloc(1,sizeof(fileMap));
    mp->output_overlay = (fileMap *) calloc(1,sizeof(fileMap));

    init_streamMap(mp->video_main);
    init_streamMap(mp->video_slide);
    init_streamMap(mp->audio_individual);
    init_fileMap(mp->output_main);
    init_fileMap(mp->output_slide);
    init_fileMap(mp->output_overlay);
}
void free_meetPro(meetPro * mp){
    av_log(NULL,AV_LOG_DEBUG,"before free_meetPro.\n");
    free_streamMap(mp->video_main);
    free_streamMap(mp->video_slide);
    free_streamMap(mp->audio_individual);
    free_fileMap(mp->output_main);
    free_fileMap(mp->output_slide);
    free_fileMap(mp->output_overlay);
    av_log(NULL,AV_LOG_DEBUG,"free_meetPro.\n");
    //free(mp);
}

