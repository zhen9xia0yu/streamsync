#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ss_stream.h>

void init_streamMap(streamMap * sm){
    /*
    av_log(NULL,AV_LOG_DEBUG,"before init_streamMap.\n");
    init_codecMap(sm->codecmap);
    init_fileMap(sm->input_fm);
    init_filterMap(sm->filtermap);
    av_log(NULL,AV_LOG_DEBUG,"init_streamMap.\n");
    */
    sm->input_fm = (fileMap *) calloc(1,sizeof(fileMap));
    sm->codecmap = (codecMap *) calloc(1,sizeof(codecMap));
    sm->filtermap = (filterMap *) calloc(1,sizeof(filterMap));

    init_fileMap(sm->input_fm);
    init_codecMap(sm->codecmap);
    init_filterMap(sm->filtermap);
   
}
void free_streamMap(streamMap * sm){
    av_log(NULL,AV_LOG_DEBUG,"before free_streamMap.\n");
    free_fileMap(sm->input_fm);
    free_codecMap(sm->codecmap);
    free_filterMap(sm->filtermap);
    av_log(NULL,AV_LOG_DEBUG,"free_streamMap.\n");
}


