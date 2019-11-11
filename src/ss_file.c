#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavformat/avformat.h>

#include <ss_file.h>
#include <ss_process.h>

void init_fileMap(fileMap * fm){
    fm->fmt_ctx = (AVFormatContext *) calloc(1,sizeof(AVFormatContext));
    fm->ofmt = (AVOutputFormat *) calloc(1,sizeof(AVOutputFormat));
}
void free_fileMap(fileMap * fm){
    av_log(NULL,AV_LOG_DEBUG,"before free_fileMap.\n");
    free(fm->fmt_ctx);
    free(fm->ofmt);
    av_log(NULL,AV_LOG_DEBUG,"free_fileMap.\n");
}

int set_inputs(meetPro * meeting){
    int ret;
    meeting->video_main->input_fm->fmt_ctx=NULL;
    meeting->audio_individual->input_fm->fmt_ctx=NULL;
    if ((ret = avformat_open_input(&meeting->video_main->input_fm->fmt_ctx, meeting->video_main->input_fm->filename, 0, &meeting->video_main->input_fm->ops)) < 0) {
        av_log(NULL,AV_LOG_ERROR,"Could not open input video file.\n");
        return ret;
    }
    if ((ret = avformat_find_stream_info(meeting->video_main->input_fm->fmt_ctx, 0)) < 0) {
        av_log(NULL,AV_LOG_ERROR, "Failed to retrieve input video stream information\n");
        return ret;
    }
    if ((ret = avformat_open_input(&meeting->audio_individual->input_fm->fmt_ctx, meeting->audio_individual->input_fm->filename, 0, &meeting->audio_individual->input_fm->ops)) < 0) {
        av_log(NULL,AV_LOG_ERROR, "Could not open input audio file.\n");
        return ret;
    }
    if ((ret = avformat_find_stream_info(meeting->audio_individual->input_fm->fmt_ctx, 0)) < 0) {
        av_log(NULL,AV_LOG_ERROR, "Failed to retrieve input audio stream information\n");
        return ret;
    }
    av_log(NULL,AV_LOG_INFO,"===========Input Information==========\n");
    av_dump_format(meeting->video_main->input_fm->fmt_ctx, 0, meeting->video_main->input_fm->filename, 0);
    av_dump_format(meeting->audio_individual->input_fm->fmt_ctx, 0, meeting->audio_individual->input_fm->filename, 0);
    av_log(NULL,AV_LOG_INFO,"======================================\n");
    return 0;
}





