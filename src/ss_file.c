#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavformat/avformat.h>

#include <ss_file.h>
#include <ss_process.h>

void init_fileMap(fileMap * fm){
    fm->fmt_ctx = (AVFormatContext *) calloc(1,sizeof(AVFormatContext));
    fm->ofmt = (AVOutputFormat *) calloc(1,sizeof(AVOutputFormat));
    av_log(NULL,AV_LOG_DEBUG,"ofmt=%x\n",fm->ofmt);
}
void free_fileMap(fileMap * fm){
    av_log(NULL,AV_LOG_DEBUG,"before free_fileMap.\n");
    free(fm->fmt_ctx);
    av_log(NULL,AV_LOG_DEBUG,"after fmt_ctx.\n");
    av_log(NULL,AV_LOG_DEBUG,"ofmt=%x\n",fm->ofmt);
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

int add_stream(meetPro *meeting,AVCodec *codec, AVCodecContext *c,enum AVCodecID codec_id){
    int i;
    int type;
    codec = avcodec_find_encoder(codec_id);
    if(!codec){
        av_log(NULL,AV_LOG_ERROR,"could not find encoder for '%s'\n",avcodec_get_name(codec_id));
        return -1;
    }else av_log(NULL,AV_LOG_DEBUG,"successed find encoder for '%s'\n",avcodec_get_name(codec_id));
    AVStream *out_stream = avformat_new_stream(meeting->output_main->fmt_ctx,NULL);
    if(!out_stream){
        av_log(NULL,AV_LOG_ERROR,"could not allocate stream\n");
        return -1;
    }else av_log(NULL,AV_LOG_DEBUG,"successed allocate stream.\n");
    c = avcodec_alloc_context3(codec);
    if(!c){
        av_log(NULL,AV_LOG_ERROR,"could not alloc an encoding context\n");
        return -1;
    }else av_log(NULL,AV_LOG_DEBUG,"successed alloc an encoding context\n");
    switch (codec->type) {
    case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt=AV_SAMPLE_FMT_FLTP;
        c->bit_rate=meeting->audio_individual->input_fm->fmt_ctx->streams[0]->codec->bit_rate;
        c->sample_rate=44100;//flv:aac supported only
        c->channel_layout=AV_CH_LAYOUT_STEREO;//flv :aac supported only, 1
        c->channels=av_get_channel_layout_nb_channels(c->channel_layout);
        c->time_base = (AVRational){1, c->sample_rate}; 
        type=1;
        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;
        c->bit_rate = 400000;
        c->width    = 1280;
        c->height   = 720;
        c->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        //ofmt_ctx->streams[0]->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        c->framerate = (AVRational){25,1};
        c->gop_size  = 12;      /* emit one intra frame every twelve frames at most */
        c->pix_fmt = STREAM_PIX_FMT;
        if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            c->max_b_frames = 2;
        }
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            c->mb_decision = 2;
       }
        type=0;
    break;
    default:
        break;
    }
    c->codec_tag = 0;
    if (meeting->output_main->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; 
    if(avcodec_open2(c,codec,NULL)<0){
        av_log(NULL,AV_LOG_ERROR,"could not open the encoder.\n");
        return -1;
    }else av_log(NULL,AV_LOG_DEBUG,"sucessed open the encoder.%d!\n",type);
    if(avcodec_copy_context(out_stream->codec, c)<0){
        av_log(NULL,AV_LOG_ERROR,"failed to set context to out stream codec\n");
        return -1;
    }else av_log(NULL,AV_LOG_DEBUG,"sucessed copy set the coder! %d !",type);
   if( avcodec_parameters_from_context(out_stream->codecpar,c)<0){
       av_log(NULL,AV_LOG_ERROR,"failed to copy codec parameters.\n");
       return -1;
   }else av_log(NULL,AV_LOG_DEBUG,"sucessed copy codec parameters.%d\n",type);
    return 0;
}

int set_outputs(meetPro *meeting){
    int ret;
    av_log(NULL,AV_LOG_DEBUG,"ofmt=%x\n",meeting->output_main->ofmt);
    avformat_alloc_output_context2(&meeting->output_main->fmt_ctx, NULL, "flv", meeting->output_main->filename);
    if (!meeting->output_main->fmt_ctx) {
        av_log(NULL,AV_LOG_ERROR, "Could not create output context\n");
        return -1;
    }
    *meeting->output_main->ofmt = *meeting->output_main->fmt_ctx->oformat;
    av_log(NULL,AV_LOG_DEBUG,"ofmt=%x\n",meeting->output_main->ofmt);
    int i;
	for (i = 0; i < meeting->video_main->input_fm->fmt_ctx->nb_streams; i++) {
		if(meeting->video_main->input_fm->fmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
		AVStream *in_stream = meeting->video_main->input_fm->fmt_ctx->streams[i];
		AVStream *out_stream = avformat_new_stream(meeting->output_main->fmt_ctx, in_stream->codec->codec);
		if (!out_stream) {
			printf( "Failed allocating output stream\n");
			ret = AVERROR_UNKNOWN;
			return -1;
		}
		if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
			printf( "Failed to copy context from input to output stream codec context\n");
			return -1;
		}
		out_stream->codec->codec_tag = 0;//与编码器相关的附加信息
		if (meeting->output_main->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
			out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
		break;
		}
	}
   if ((ret = add_stream(meeting,meeting->audio_individual->codecmap->codec,
                        meeting->audio_individual->codecmap->codec_ctx,AUDIO_CODEC_ID)) < 0){
       av_log(NULL,AV_LOG_ERROR,"error occured when add audio stream failed.\n");
       return -1;
   }
    av_log(NULL,AV_LOG_INFO,"==========Output Information==========\n");
    av_dump_format(meeting->output_main->fmt_ctx, 0, meeting->output_main->filename, 1);
    av_log(NULL,AV_LOG_INFO,"======================================\n");
    //Open output file
    if (!(meeting->output_main->ofmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&meeting->output_main->fmt_ctx->pb, meeting->output_main->filename, AVIO_FLAG_WRITE) < 0) {
            av_log(NULL,AV_LOG_ERROR, "Could not open output file '%s'", meeting->output_main->filename);
            return -1;
        }
    }
    //Write file header
    if (avformat_write_header(meeting->output_main->fmt_ctx, NULL) < 0) {
        av_log(NULL,AV_LOG_ERROR, "could not write the header to output.\n");
        return -1;
    }
    return 0;
}

int set_decoder(AVFormatContext *ctx, int stream_id, AVCodec *codec,AVCodecContext *c){
   codec = avcodec_find_decoder(ctx->streams[stream_id]->codec->codec_id);
   if(!codec){
       av_log(NULL,AV_LOG_ERROR,"could not find decodec for '%s'\n",avcodec_get_name(ctx->streams[stream_id]->codec->codec_id));
       return -1;
   }
   c = avcodec_alloc_context3(codec);
   c->thread_count=1;//????
   avcodec_copy_context(c,ctx->streams[stream_id]->codec);
   if(avcodec_open2(c,NULL,NULL)<0){
       av_log(NULL,AV_LOG_ERROR,"could not open the codec '%s'\n",avcodec_get_name(ctx->streams[stream_id]->codec->codec_id));
       return -1;
    }else   av_log(NULL,AV_LOG_DEBUG,"seccessed open the decoder '%s'\n",avcodec_get_name(ctx->streams[stream_id]->codec->codec_id));
   return 0;
}



