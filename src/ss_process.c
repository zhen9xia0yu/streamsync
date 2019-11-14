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
    free_streamMap(mp->video_main);
    free_streamMap(mp->video_slide);
    free_streamMap(mp->audio_individual);
    free_fileMap(mp->output_main);
    free_fileMap(mp->output_slide);
    free_fileMap(mp->output_overlay);
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

int add_stream(meetPro *meeting,codecMap *cm,enum AVCodecID codec_id){
    int i;
    int type;
    cm->codec = avcodec_find_encoder(codec_id);
    if(!cm->codec){
        av_log(NULL,AV_LOG_ERROR,"could not find encoder for '%s'\n",avcodec_get_name(codec_id));
        return -1;
    }else av_log(NULL,AV_LOG_DEBUG,"successed find encoder for '%s'\n",avcodec_get_name(codec_id));
    AVStream *out_stream = avformat_new_stream(meeting->output_main->fmt_ctx,NULL);
    if(!out_stream){
        av_log(NULL,AV_LOG_ERROR,"could not allocate stream\n");
        return -1;
    }else av_log(NULL,AV_LOG_DEBUG,"successed allocate stream.\n");
    cm->codec_ctx = avcodec_alloc_context3(cm->codec);
    if(!cm->codec_ctx){
        av_log(NULL,AV_LOG_ERROR,"could not alloc an encoding context\n");
        return -1;
    }else av_log(NULL,AV_LOG_DEBUG,"successed alloc an encoding context\n");
    switch (cm->codec->type) {
    case AVMEDIA_TYPE_AUDIO:
        cm->codec_ctx->sample_fmt=AV_SAMPLE_FMT_FLTP;
        cm->codec_ctx->bit_rate=meeting->audio_individual->input_fm->fmt_ctx->streams[0]->codec->bit_rate;
        cm->codec_ctx->sample_rate=44100;//flv:aac supported only
        cm->codec_ctx->channel_layout=AV_CH_LAYOUT_STEREO;//flv :aac supported only, 1
        cm->codec_ctx->channels=av_get_channel_layout_nb_channels(cm->codec_ctx->channel_layout);
        cm->codec_ctx->time_base = (AVRational){1, cm->codec_ctx->sample_rate}; 
        
        type=1;
        break;
    case AVMEDIA_TYPE_VIDEO:
        cm->codec_ctx->codec_id = codec_id;
        cm->codec_ctx->bit_rate = 400000;
        cm->codec_ctx->width    = 1280;
        cm->codec_ctx->height   = 720;
        cm->codec_ctx->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        //ofmt_ctx->streams[0]->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        cm->codec_ctx->framerate = (AVRational){25,1};
        cm->codec_ctx->gop_size  = 12;      /* emit one intra frame every twelve frames at most */
        cm->codec_ctx->pix_fmt = STREAM_PIX_FMT;
        if (cm->codec_ctx->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
            cm->codec_ctx->max_b_frames = 2;
        }
        if (cm->codec_ctx->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
            cm->codec_ctx->mb_decision = 2;
       }
        type=0;
    break;
    default:
        break;
    }
    cm->codec_ctx->codec_tag = 0;
    if (meeting->output_main->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        cm->codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; 
    if(avcodec_open2(cm->codec_ctx,cm->codec,NULL)<0){
        av_log(NULL,AV_LOG_ERROR,"could not open the encoder.\n");
        return -1;
    }else av_log(NULL,AV_LOG_DEBUG,"sucessed open the encoder.%d!\n",type);
    if(avcodec_copy_context(out_stream->codec, cm->codec_ctx)<0){
        av_log(NULL,AV_LOG_ERROR,"failed to set context to out stream codec\n");
        return -1;
    }else av_log(NULL,AV_LOG_DEBUG,"sucessed copy set the coder! %d !",type);
   if( avcodec_parameters_from_context(out_stream->codecpar,cm->codec_ctx)<0){
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
    if ((ret = add_stream(meeting,meeting->audio_individual->codecmap,AUDIO_CODEC_ID)) < 0){
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

