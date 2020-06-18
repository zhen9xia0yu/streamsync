#include <ss_process.h>

void init_meetPro(meetPro * mp){
    /*
     * 初始化结构体指针，从外到内。
    */
    mp->video = (streamMap *) calloc(1,sizeof(streamMap));
    mp->video_auxi = (streamMap *) calloc(1,sizeof(streamMap));
    mp->audio = (streamMap *) calloc(1,sizeof(streamMap));
    mp->output = (fileMap *) calloc(1,sizeof(fileMap));
    mp->output_auxi = (fileMap *) calloc(1,sizeof(fileMap));
    init_streamMap(mp->video);
    init_streamMap(mp->video_auxi);
    init_streamMap(mp->audio);
    init_fileMap(mp->output);
    init_fileMap(mp->output_auxi);
}

void free_meetPro(meetPro * mp){
    free_streamMap(mp->video);
    free_streamMap(mp->video_auxi);
    free_streamMap(mp->audio);
    free_fileMap(mp->output);
    free_fileMap(mp->output_auxi);
}

int set_inputs(meetPro * meeting){
    int ret;
    meeting->video->input_fm->fmt_ctx=NULL;
    meeting->video_auxi->input_fm->fmt_ctx=NULL;
    meeting->audio->input_fm->fmt_ctx=NULL;
    if ((ret = avformat_open_input(&meeting->video->input_fm->fmt_ctx, meeting->video->input_fm->filename, 0, &meeting->video->input_fm->ops)) < 0) {
        av_log(NULL,AV_LOG_ERROR,"Could not open input video file.\n");
        return ret;
    }
    if ((ret = avformat_open_input(&meeting->video_auxi->input_fm->fmt_ctx, meeting->video->input_fm->filename, 0, &meeting->video->input_fm->ops)) < 0) {
        av_log(NULL,AV_LOG_ERROR,"Could not open input video file.\n");
        return ret;
    }
    if ((ret = avformat_open_input(&meeting->audio->input_fm->fmt_ctx, meeting->audio->input_fm->filename, 0, &meeting->audio->input_fm->ops)) < 0) {
        av_log(NULL,AV_LOG_ERROR, "Could not open input audio file.\n");
        return ret;
    }
    if ((ret = avformat_find_stream_info(meeting->video->input_fm->fmt_ctx, 0)) < 0) {
        av_log(NULL,AV_LOG_ERROR, "Failed to retrieve input video stream information\n");
        return ret;
    }
    if ((ret = avformat_find_stream_info(meeting->video_auxi->input_fm->fmt_ctx, 0)) < 0) {
        av_log(NULL,AV_LOG_ERROR, "Failed to retrieve input video stream information\n");
        return ret;
    }
    if ((ret = avformat_find_stream_info(meeting->audio->input_fm->fmt_ctx, 0)) < 0) {
        av_log(NULL,AV_LOG_ERROR, "Failed to retrieve input audio stream information\n");
        return ret;
    }
    av_log(NULL,AV_LOG_INFO,"===========Input Information==========\n");
    av_dump_format(meeting->video->input_fm->fmt_ctx, 0, meeting->video->input_fm->filename, 0);
    av_dump_format(meeting->video_auxi->input_fm->fmt_ctx, 0, meeting->video->input_fm->filename, 0);
    av_dump_format(meeting->audio->input_fm->fmt_ctx, 0, meeting->audio->input_fm->filename, 0);
    av_log(NULL,AV_LOG_INFO,"======================================\n");
    return 0;
}

int add_stream(meetPro *meeting,codecMap *cm,enum AVCodecID codec_id,const char *bitrate){
    int i;
    int type;
    cm->opts=NULL;
    AVStream * s=meeting->video->input_fm->fmt_ctx->streams[0];
    cm->codec = avcodec_find_encoder(codec_id);
    if(!cm->codec){
        av_log(NULL,AV_LOG_ERROR,"could not find encoder for '%s'\n",avcodec_get_name(codec_id));
        return -1;
    }else av_log(NULL,AV_LOG_DEBUG,"successed find encoder for '%s'\n",avcodec_get_name(codec_id));
    AVStream *out_stream = avformat_new_stream(meeting->output->fmt_ctx,NULL);
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
        cm->codec_ctx->bit_rate=meeting->audio->input_fm->fmt_ctx->streams[0]->codec->bit_rate;
       // av_dict_set(&cm->opts,"b","8000",0);
        cm->codec_ctx->sample_rate=44100;//flv:aac supported only
        //cm->codec_ctx->channel_layout=AV_CH_LAYOUT_MONO;
        cm->codec_ctx->channel_layout=AV_CH_LAYOUT_STEREO;//flv :aac supported only, 1
        cm->codec_ctx->channels=av_get_channel_layout_nb_channels(cm->codec_ctx->channel_layout);
        cm->codec_ctx->time_base = (AVRational){1, cm->codec_ctx->sample_rate};
        meeting->output->fmt_ctx->streams[1]->time_base=cm->codec_ctx->time_base;
        type=1;
        break;
    case AVMEDIA_TYPE_VIDEO:
        /*
        cm->codec_ctx->codec_id = codec_id;
        cm->codec_ctx->bit_rate = 400000;
        cm->codec_ctx->width    = 1280;
        cm->codec_ctx->height   = 720;
        cm->codec_ctx->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        //ofmt_ctx->streams[0]->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        cm->codec_ctx->framerate = (AVRational){25,1};
        cm->codec_ctx->pix_fmt = STREAM_PIX_FMT;
        cm->codec_ctx->gop_size  = 12; */     /* emit one intra frame every twelve frames at most */

        cm->codec_ctx->codec_id = codec_id;
        cm->codec_ctx->bit_rate = s->codec->bit_rate;
        cm->codec_ctx->width    = s->codec->width;
        cm->codec_ctx->height   = s->codec->height;
        //meeting->output->fmt_ctx->streams[0]->time_base = (AVRational){ 1, STREAM_FRAME_RATE };
        cm->codec_ctx->time_base = s->codec->time_base;//原tbc
        //meeting->output->fmt_ctx->streams[0]->time_base=cm->codec_ctx->time_base;
        meeting->output->fmt_ctx->streams[0]->time_base=s->time_base;//原tbn
        meeting->output->fmt_ctx->streams[0]->avg_frame_rate=s->avg_frame_rate;//原fps
        meeting->output->fmt_ctx->streams[0]->r_frame_rate=s->r_frame_rate;//原tbr
        //cm->codec_ctx->framerate = (AVRational){25,1};
        cm->codec_ctx->framerate = s->codec->framerate;//
        cm->codec_ctx->pix_fmt = STREAM_PIX_FMT;
        //cm->codec_ctx->gop_size  = 12;      /* emit one intra frame every twelve frames at most */
        cm->codec_ctx->gop_size  = 25;      /* emit one intra frame every twelve frames at most */
        av_dict_set(&cm->opts,"minrate",bitrate,0);
        av_dict_set(&cm->opts,"b",bitrate,0);
        av_dict_set(&cm->opts,"bufsize",bitrate,0);
        av_dict_set(&cm->opts,"maxrate",bitrate,0);
        av_dict_set(&cm->opts,"profile","baseline",0);
        av_dict_set(&cm->opts,"level","3",0);
        av_dict_set(&cm->opts,"strict","2",0);
        if(codec_id == AV_CODEC_ID_H264)
            av_opt_set(cm->codec_ctx->priv_data,"preset","superfast",0);
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
    if (meeting->output->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        cm->codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    if(avcodec_open2(cm->codec_ctx,cm->codec,&cm->opts)<0){
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

int set_outputs(meetPro *meeting,int trans_video,int trans_audio, const char *bitrate){
    int i,ret;
    av_log(NULL,AV_LOG_DEBUG,"ofmt=%x\n",meeting->output->ofmt);
    av_log(NULL,AV_LOG_DEBUG,"ofmt_auxi=%x\n",meeting->output_auxi->ofmt);

    avformat_alloc_output_context2(&meeting->output->fmt_ctx, NULL, "flv", meeting->output->filename);
    if (!meeting->output->fmt_ctx) {
        av_log(NULL,AV_LOG_ERROR, "Could not create output main context\n");
        return -1;
    }

    avformat_alloc_output_context2(&meeting->output_auxi->fmt_ctx, NULL, "flv", meeting->output_auxi->filename);
    if (!meeting->output_auxi->fmt_ctx) {
        av_log(NULL,AV_LOG_ERROR, "Could not create output auxiliary context\n");
        return -1;
    }

    *meeting->output->ofmt = *meeting->output->fmt_ctx->oformat;
    *meeting->output_auxi->ofmt = *meeting->output_auxi->fmt_ctx->oformat;
    av_log(NULL,AV_LOG_DEBUG,"ofmt=%x\n",meeting->output->ofmt);
    av_log(NULL,AV_LOG_DEBUG,"ofmt_auxi=%x\n",meeting->output_auxi->ofmt);
    if(!trans_video){
        for (i = 0; i < meeting->video->input_fm->fmt_ctx->nb_streams; i++) {
            if(meeting->video->input_fm->fmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
                AVStream *in_stream = meeting->video->input_fm->fmt_ctx->streams[i];
                AVStream *out_stream = avformat_new_stream(meeting->output->fmt_ctx, in_stream->codec->codec);
                if (!out_stream) {
                    av_log(NULL,AV_LOG_ERROR, "Failed allocating output stream\n");
                    ret = AVERROR_UNKNOWN;
                    return -1;
                }
                if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
                    av_log(NULL,AV_LOG_ERROR, "Failed to copy context from input to output stream codec context\n");
                    return -1;
                }
                out_stream->codec->codec_tag = 0;//与编码器相关的附加信息
                if (meeting->output->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                    out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
                break;
            }
        }

        for (i = 0; i < meeting->video_auxi->input_fm->fmt_ctx->nb_streams; i++) {
            if(meeting->video_auxi->input_fm->fmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
                AVStream *in_stream = meeting->video_auxi->input_fm->fmt_ctx->streams[i];
                AVStream *out_stream = avformat_new_stream(meeting->output_auxi->fmt_ctx, in_stream->codec->codec);
                if (!out_stream) {
                    av_log(NULL,AV_LOG_ERROR, "Failed allocating output auxiliary stream\n");
                    ret = AVERROR_UNKNOWN;
                    return -1;
                }
                if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
                    av_log(NULL,AV_LOG_ERROR, "Failed to copy context from input to output stream codec context\n");
                    return -1;
                }
                out_stream->codec->codec_tag = 0;//与编码器相关的附加信息
                if (meeting->output_auxi->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                    out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
                break;
            }
        }


    }else{
        if ((ret = add_stream(meeting,meeting->video->codecmap,VIDEO_CODEC_ID,bitrate)) < 0){
            av_log(NULL,AV_LOG_ERROR,"error occured when add video stream failed.\n");
            return -1;
        }
        if ((ret = add_stream(meeting,meeting->video_auxi->codecmap,VIDEO_CODEC_ID,bitrate)) < 0){
            av_log(NULL,AV_LOG_ERROR,"error occured when add video stream failed.\n");
            return -1;
        }
    }
    if(!trans_audio){
        for (i = 0; i < meeting->audio->input_fm->fmt_ctx->nb_streams; i++) {
            if(meeting->audio->input_fm->fmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_AUDIO){
                AVStream *in_stream = meeting->audio->input_fm->fmt_ctx->streams[i];
                AVStream *out_stream = avformat_new_stream(meeting->output->fmt_ctx, in_stream->codec->codec);
                AVStream *out_stream_auxi = avformat_new_stream(meeting->output_auxi->fmt_ctx, in_stream->codec->codec);
                if (!out_stream) {
                    printf( "Failed allocating output stream\n");
                    ret = AVERROR_UNKNOWN;
                    return -1;
                }
                if (!out_stream_auxi) {
                    printf( "Failed allocating output stream\n");
                    ret = AVERROR_UNKNOWN;
                    return -1;
                }

                if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
                    printf( "Failed to copy context from input to output stream codec context\n");
                    return -1;
                }
                if (avcodec_copy_context(out_stream_auxi->codec, in_stream->codec) < 0) {
                    printf( "Failed to copy context from input to output stream codec context\n");
                    return -1;
                }

                out_stream->codec->codec_tag = 0;//与编码器相关的附加信息
                out_stream_auxi->codec->codec_tag = 0;//与编码器相关的附加信息
                if (meeting->output->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                    out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
                break;
                if (meeting->output_auxi->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                    out_stream_auxi->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
                break;

            }
        }

    }
    else{
        if ((ret = add_stream(meeting,meeting->audio->codecmap,AUDIO_CODEC_ID,bitrate)) < 0){
          av_log(NULL,AV_LOG_ERROR,"error occured when add audio stream failed.\n");
           return -1;
        }
        if ((ret = add_stream(meeting,meeting->audio->codecmap,AUDIO_CODEC_ID,bitrate)) < 0){
          av_log(NULL,AV_LOG_ERROR,"error occured when add audio stream failed.\n");
           return -1;
        }

    }
    av_log(NULL,AV_LOG_INFO,"==========Output Information==========\n");
    av_dump_format(meeting->output->fmt_ctx, 0, meeting->output->filename, 1);
    av_dump_format(meeting->output_auxi->fmt_ctx, 0, meeting->output->filename, 1);
    av_log(NULL,AV_LOG_INFO,"======================================\n");
    //Open output file
    if (!(meeting->output->ofmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&meeting->output->fmt_ctx->pb, meeting->output->filename, AVIO_FLAG_WRITE) < 0) {
            av_log(NULL,AV_LOG_ERROR, "Could not open output file '%s'", meeting->output->filename);
            return -1;
        }
    }
    if (!(meeting->output_auxi->ofmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&meeting->output_auxi->fmt_ctx->pb, meeting->output_auxi->filename, AVIO_FLAG_WRITE) < 0) {
            av_log(NULL,AV_LOG_ERROR, "Could not open output file '%s'", meeting->output->filename);
            return -1;
        }
    }

    //Write file header
    if (avformat_write_header(meeting->output->fmt_ctx, NULL) < 0) {
        av_log(NULL,AV_LOG_ERROR, "could not write the header to output.\n");
        return -1;
    }
    if (avformat_write_header(meeting->output_auxi->fmt_ctx, NULL) < 0) {
        av_log(NULL,AV_LOG_ERROR, "could not write the header to output.\n");
        return -1;
    }

    return 0;
}

