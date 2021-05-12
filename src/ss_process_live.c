#include <ss_process_live.h>

void init_LivePro(LivePro *livep){
    /*
     * 初始化结构体指针，从外到内。
    */
    livep->input_rtmp   = (fileMap *) calloc(1,sizeof(fileMap));
    livep->output_video = (fileMap *) calloc(1,sizeof(fileMap));
    livep->output_audio = (fileMap *) calloc(1,sizeof(fileMap));
    livep->video = (streamMap *) calloc(1,sizeof(streamMap));
    livep->audio = (streamMap *) calloc(1,sizeof(streamMap));
    livep->rtmp_index_video = -1;
    livep->rtmp_index_audio = -1;
    
    init_fileMap(livep->input_rtmp);
    init_fileMap(livep->output_video);
    init_fileMap(livep->output_audio);
    init_streamMap(livep->video);
    init_streamMap(livep->audio);
}

void free_LivePro(LivePro * livep){
    free_fileMap(livep->input_rtmp);
    free_fileMap(livep->output_video);
    free_fileMap(livep->output_audio);
    free_streamMap(livep->video);
    free_streamMap(livep->audio);
}

int set_inputs(LivePro * livep){
    int ret,i;
    if(livep->input_rtmp->filename != ""){
    	livep->input_rtmp->fmt_ctx=NULL; 
	if ((ret = avformat_open_input(&livep->input_rtmp->fmt_ctx, livep->input_rtmp->filename, 0, &livep->input_rtmp->ops)) < 0) {
    	    av_log(NULL,AV_LOG_ERROR,"Could not open input URL.\n");
    	    return ret;
    	}
   	if ((ret = avformat_find_stream_info(livep->input_rtmp->fmt_ctx, 0)) < 0) {
   	    av_log(NULL,AV_LOG_ERROR, "Failed to find input stream information\n");
   	    return ret;
   	}
	for (i=0; i<livep->input_rtmp->fmt_ctx->nb_streams; i++){
	    if( livep->input_rtmp->fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		livep->rtmp_index_video = i;
	    if( livep->input_rtmp->fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
		livep->rtmp_index_audio = i;
	}
    	av_log(NULL,AV_LOG_INFO,"==============Input Information=============\n");
    	av_dump_format(livep->input_rtmp->fmt_ctx, 0, livep->input_rtmp->filename, 0);
	av_log(NULL,AV_LOG_INFO,"RTMP video stream index: %d\n", livep->rtmp_index_video);
	av_log(NULL,AV_LOG_INFO,"RTMP audio stream index: %d\n", livep->rtmp_index_audio);
    	av_log(NULL,AV_LOG_INFO,"============================================\n");
    }else{
    	av_log(NULL,AV_LOG_ERROR,"input URL none.\n");
	return -1;
    }
    return 0;
}

int set_outputs(LivePro *livep, int flags_video_trans){
    int ret;
    /************video start**************/
    avformat_alloc_output_context2(&livep->output_video->fmt_ctx, NULL, "rtp", livep->output_video->filename);
    if (!livep->output_video->fmt_ctx) {
        av_log(NULL,AV_LOG_ERROR, "Could not create output video context\n");
        return -1;
    }

    if(flags_video_trans) {
	//copy stream
	AVStream *in_stream = livep->input_rtmp->fmt_ctx->streams[ livep->rtmp_index_video ];
	AVStream *out_stream = avformat_new_stream(livep->output_video->fmt_ctx, in_stream->codec->codec);
	if (!out_stream) {
	    printf( "Failed allocating output video stream\n");
	    ret = AVERROR_UNKNOWN;
	    return -1;
	}
	if (avcodec_parameters_from_context(out_stream->codecpar, in_stream->codec) < 0) {
	    printf( "Failed to copy context from input to output video stream codec context\n");
	    return -1;
	}
	out_stream->codec->codec_tag = 0;//与编码器相关的附加信息
	if (livep->output_video->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER){
	    out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}
    }
    else{
	if ((ret = add_stream(meeting,meeting->audio->codecmap,AUDIO_CODEC_ID,bitrate)) < 0){
    	  av_log(NULL,AV_LOG_ERROR,"error occured when add video stream failed.\n");
    	   return -1;
    	}
    }
	
    av_log(NULL,AV_LOG_INFO,"==========Output video Information==========\n");
    av_dump_format(livep->output_video->fmt_ctx, 0, livep->output_video->filename, 1);
    av_log(NULL,AV_LOG_INFO,"============================================\n");
    //Open output video file
    if (!(livep->output_video->ofmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&livep->output_video->fmt_ctx->pb, livep->output_video->filename, AVIO_FLAG_WRITE) < 0) {
            av_log(NULL,AV_LOG_ERROR, "Could not open output video file '%s'", livep->output_video->filename);
            return -1;
        }
    }
    //Write output video file header
    if (avformat_write_header(livep->output_video->fmt_ctx, NULL) < 0) {
        av_log(NULL,AV_LOG_ERROR, "could not write the header to output video.\n");
        return -1;
    }
    /************video over**************/

    /************audio start**************/
    avformat_alloc_output_context2(&livep->output_audio->fmt_ctx, NULL, "rtp", livep->output_audio->filename);
    if (!livep->output_audio->fmt_ctx) {
        av_log(NULL,AV_LOG_ERROR, "Could not create output audio context\n");
        return -1;
    }

	//copy stream
	in_stream = livep->input_rtmp->fmt_ctx->streams[ livep->rtmp_index_audio ];
	out_stream = avformat_new_stream(livep->output_audio->fmt_ctx, in_stream->codec->codec);
	if (!out_stream) {
	    printf( "Failed allocating output audio stream\n");
	    ret = AVERROR_UNKNOWN;
	    return -1;
	}
	if (avcodec_parameters_from_context(out_stream->codecpar, in_stream->codec) < 0) {
	    printf( "Failed to copy context from input to output audio stream codec context\n");
	    return -1;
	}
	out_stream->codec->codec_tag = 0;//与编码器相关的附加信息
	if (livep->output_audio->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER){
	    out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
	}

    av_log(NULL,AV_LOG_INFO,"==========Output audio Information==========\n");
    av_dump_format(livep->output_audio->fmt_ctx, 0, livep->output_audio->filename, 1);
    av_log(NULL,AV_LOG_INFO,"============================================\n");
    //Open output audio file
    if (!(livep->output_audio->ofmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&livep->output_audio->fmt_ctx->pb, livep->output_audio->filename, AVIO_FLAG_WRITE) < 0) {
            av_log(NULL,AV_LOG_ERROR, "Could not open output audio file '%s'", livep->output_audio->filename);
            return -1;
        }
    }
    //Write output audio file header
    if (avformat_write_header(livep->output_audio->fmt_ctx, NULL) < 0) {
        av_log(NULL,AV_LOG_ERROR, "could not write the header to output audio.\n");
        return -1;
    }
    /************audio over**************/


    return 0;
}

int add_stream( fileMap *input_fm, fileMap *output_fm,codecMap *cm,enum AVCodecID codec_id){
    int i;
    cm->opts=NULL;
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

	AVStream * s=meeting->video->input_fm->fmt_ctx->streams[0];
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
       av_log(null,av_log_error,"failed to copy codec parameters.\n");
       return -1;
   }else av_log(null,av_log_debug,"sucessed copy codec parameters.%d\n",type);
    return 0;
}

//int set_outputs(meetPro *meeting,int trans_video,const char *bitrate){
//    int ret;
//    av_log(NULL,AV_LOG_DEBUG,"ofmt=%x\n",meeting->output->ofmt);
//    //avformat_alloc_output_context2(&meeting->output->fmt_ctx, NULL, "flv", meeting->output->filename);
//    avformat_alloc_output_context2(&meeting->output->fmt_ctx, NULL, "rtp", meeting->output->filename);
//    if (!meeting->output->fmt_ctx) {
//        av_log(NULL,AV_LOG_ERROR, "Could not create output context\n");
//        return -1;
//    }
//    *meeting->output->ofmt = *meeting->output->fmt_ctx->oformat;
//    av_log(NULL,AV_LOG_DEBUG,"ofmt=%x\n",meeting->output->ofmt);
//    if(meeting->video->input_fm->filename != ""){
//    	if(!trans_video){
//    	    int i;
//    	    for (i = 0; i < meeting->video->input_fm->fmt_ctx->nb_streams; i++) {
//    	        if(meeting->video->input_fm->fmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
//    	        AVStream *in_stream = meeting->video->input_fm->fmt_ctx->streams[i];
//    	        AVStream *out_stream = avformat_new_stream(meeting->output->fmt_ctx, in_stream->codec->codec);
//    	        if (!out_stream) {
//    	            printf( "Failed allocating output stream\n");
//    	            ret = AVERROR_UNKNOWN;
//    	            return -1;
//    	        }
//    	        if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
//    	            printf( "Failed to copy context from input to output stream codec context\n");
//    	            return -1;
//    	        }
//    	        out_stream->codec->codec_tag = 0;//与编码器相关的附加信息
//    	        if (meeting->output->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
//    	            out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
//    	        break;
//    	        }
//    	    }
//    	}else{
//    	    if ((ret = add_stream(meeting,meeting->video->codecmap,VIDEO_CODEC_ID,bitrate)) < 0){
//    	        av_log(NULL,AV_LOG_ERROR,"error occured when add video stream failed.\n");
//    	        return -1;
//    	    }
//    	}
//    }
//    if(meeting->audio->input_fm->filename != ""){
//    	if ((ret = add_stream(meeting,meeting->audio->codecmap,AUDIO_CODEC_ID,bitrate)) < 0){
//    	  av_log(NULL,AV_LOG_ERROR,"error occured when add audio stream failed.\n");
//    	   return -1;
//    	}
//    }
//    av_log(NULL,AV_LOG_INFO,"==========Output Information==========\n");
//    av_dump_format(meeting->output->fmt_ctx, 0, meeting->output->filename, 1);
//    av_log(NULL,AV_LOG_INFO,"======================================\n");
//    //Open output file
//    if (!(meeting->output->ofmt->flags & AVFMT_NOFILE)) {
//        if (avio_open(&meeting->output->fmt_ctx->pb, meeting->output->filename, AVIO_FLAG_WRITE) < 0) {
//            av_log(NULL,AV_LOG_ERROR, "Could not open output file '%s'", meeting->output->filename);
//            return -1;
//        }
//    }
//    //Write file header
//    if (avformat_write_header(meeting->output->fmt_ctx, NULL) < 0) {
//        av_log(NULL,AV_LOG_ERROR, "could not write the header to output.\n");
//        return -1;
//    }
//    return 0;
//}
//
