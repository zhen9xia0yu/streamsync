#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ss_process.h"

#define USE_H264BSF 1
#define USE_AACBSF 1

int main(int argc,char **argv){
    int ret,i;
    meetPro *meeting;
    AVCodecParserContext * parser;
    FILE *f;
    FILE *op_f;
    long inbuf_size;
    uint8_t *data;
    size_t data_size;
    AVPacket *pkt;
    const char *ip_filename;
    const char *op_filename;
    AVBitStreamFilterContext* h264bsfc = av_bitstream_filter_init("h264_mp4toannexb");
    unsigned char *dummy=NULL;   //输入的指针
    int dummy_len;
    char nal_start[]={0,0,0,1};

    if(argc!=3){
        //av_log(NULL,AV_LOG_ERROR,"usage: %s <input video file> rtp://?:?\n",argv[0]);
        av_log(NULL,AV_LOG_ERROR,"usage: %s <input video file> <output video file>\n",argv[0]);
        return -1;
    }
    //open & read file at once
    ip_filename = argv[1];
    op_filename = argv[2];
    f = fopen(ip_filename,"rb");
    op_f = fopen(op_filename,"ab");
    if(!f){
        av_log(NULL,AV_LOG_ERROR,"cannot open %s\n",ip_filename);
        return -1;
    }
    if(!op_f){
        av_log(NULL,AV_LOG_ERROR,"cannot open %s\n",op_filename);
        return -1;
    }
    fseek(f,0,SEEK_END);
    inbuf_size=ftell(f);
    rewind(f);
    uint8_t inbuf[inbuf_size + AV_INPUT_BUFFER_PADDING_SIZE];
    memset(inbuf + inbuf_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    //init
    av_log_set_level(AV_LOG_DEBUG);
    av_register_all();
    avformat_network_init();
    meeting = (meetPro *) calloc(1,sizeof(meetPro));
    init_meetPro(meeting);
    pkt= av_packet_alloc();
    if(!pkt){
        av_log(NULL,AV_LOG_ERROR,"cannot alloc packet.\n");
        goto end;
    }
    //setting default values
    meeting->video->input_fm->filename=argv[1];
    meeting->output->filename=argv[2];
    meeting->video->cur_pts=0;
    meeting->video->cur_index_pkt_in=0;
    meeting->video->input_fm->fmt_ctx=NULL;
    if ((ret = avformat_open_input(&meeting->video->input_fm->fmt_ctx, meeting->video->input_fm->filename, 0, &meeting->video->input_fm->ops)) < 0) {
        av_log(NULL,AV_LOG_ERROR,"Could not open input video file.\n");
        goto end;
    }
    if ((ret = avformat_find_stream_info(meeting->video->input_fm->fmt_ctx, 0)) < 0) {
        av_log(NULL,AV_LOG_ERROR, "Failed to retrieve input video stream information\n");
        goto end;
    }
    av_log(NULL,AV_LOG_INFO,"===========Input Information==========\n");
    av_dump_format(meeting->video->input_fm->fmt_ctx, 0, meeting->video->input_fm->filename, 0);
    av_log(NULL,AV_LOG_INFO,"======================================\n");
    //set output
    //avformat_alloc_output_context2(&meeting->output->fmt_ctx, NULL, "rtp", meeting->output->filename);
    avformat_alloc_output_context2(&meeting->output->fmt_ctx, NULL,NULL, meeting->output->filename);
    if (!meeting->output->fmt_ctx) {
        av_log(NULL,AV_LOG_ERROR, "Could not create output context\n");
        goto end;
    }
    *meeting->output->ofmt = *meeting->output->fmt_ctx->oformat;
    av_log(NULL,AV_LOG_DEBUG,"ofmt=%x\n",meeting->output->ofmt);
    for (i = 0; i < meeting->video->input_fm->fmt_ctx->nb_streams; i++) {
        if(meeting->video->input_fm->fmt_ctx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO){
            AVStream *in_stream = meeting->video->input_fm->fmt_ctx->streams[i];
            AVStream *out_stream = avformat_new_stream(meeting->output->fmt_ctx, in_stream->codec->codec);
            if (!out_stream) {
                printf( "Failed allocating output stream\n");
                ret = AVERROR_UNKNOWN;
                goto end;
            }
            if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
                printf( "Failed to copy context from input to output stream codec context\n");
                goto end;
            }
            out_stream->codec->codec_tag = 0;//与编码器相关的附加信息
            if (meeting->output->fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
                out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
            break;
        }
    }
    av_log(NULL,AV_LOG_INFO,"==========Output Information==========\n");
    av_dump_format(meeting->output->fmt_ctx, 0, meeting->output->filename, 1);
    av_log(NULL,AV_LOG_INFO,"======================================\n");
    //Open output file
    if (!(meeting->output->ofmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&meeting->output->fmt_ctx->pb, meeting->output->filename, AVIO_FLAG_WRITE) < 0) {
            av_log(NULL,AV_LOG_ERROR, "Could not open output file '%s'", meeting->output->filename);
            goto end;
        }
    }
    //Write file header
    if (avformat_write_header(meeting->output->fmt_ctx, NULL) < 0) {
        av_log(NULL,AV_LOG_ERROR, "could not write the header to output.\n");
        goto end;
    }
   //init
    AVFormatContext *ifmt_ctx;
    AVStream *in_stream, *out_stream;
    streamMap *sm_v_main = meeting->video;
    ifmt_ctx=sm_v_main->input_fm->fmt_ctx;
    in_stream=ifmt_ctx->streams[0];
    out_stream=meeting->output->fmt_ctx->streams[0];
    parser = av_parser_init(sm_v_main->input_fm->fmt_ctx->streams[0]->codec->codec_id);
    if(!parser){
        av_log(NULL,AV_LOG_ERROR,"parser not found\n");
        goto end;
    }

    //ready to delay
    int64_t start_time=0;
    start_time=av_gettime();
    AVRational time_base;
    AVRational time_base_q = {1,AV_TIME_BASE};
    int64_t pts_time;
    int64_t now_time;
    size_t save_size;
    data_size = fread(inbuf,1,inbuf_size, f);
    fclose(f);
    if(!data_size){
        ret=-1;
        goto end;
    }
    save_size=data_size;
    av_log(NULL,AV_LOG_DEBUG,"file_size = %zu\n",save_size);

    //write pps fps
    av_log(NULL,AV_LOG_DEBUG,"before filter_filter\n");
    av_bitstream_filter_filter(h264bsfc,meeting->output->fmt_ctx->streams[0]->codec,
                                NULL, &dummy, &dummy_len, NULL, 0, 0);
    //fwrite(pCodecCtx->extradata,pCodecCtx-->extradata_size,1,fp);
    av_log(NULL,AV_LOG_DEBUG,"before fwrite codec\n");
//    fwrite(meeting->output->fmt_ctx->streams[0]->codec->extradata,
//           meeting->output->fmt_ctx->streams[0]->codec->extradata_size,1,op_f);
    av_bitstream_filter_close(h264bsfc);
    free(dummy);


//    while(1){
    data = inbuf;
    data_size=save_size;
    while(data_size>0){
        ret =  av_parser_parse2(parser,in_stream->codec,&pkt->data,&pkt->size,
                                data,data_size,AV_NOPTS_VALUE,AV_NOPTS_VALUE,0);
        if(ret < 0){
            av_log(NULL,AV_LOG_ERROR,"error while parsing\n");
            goto end;
        }
        data      +=ret;
        data_size -=ret;
        if(pkt->size){
            ret = set_pts(pkt,in_stream,sm_v_main->cur_index_pkt_in);
            av_log(NULL,AV_LOG_INFO,"set the vpkt pts:%"PRId64" \n",pkt->pts);
            if(ret<0){
                av_log(NULL,AV_LOG_ERROR,"could not set pts\n");
                goto end;
            }
            sm_v_main->cur_index_pkt_in++;
            sm_v_main->cur_pts=pkt->pts;

            fwrite(nal_start,4,1,op_f);
            fwrite(pkt->data+4,pkt->size-4,1,op_f);

    //           ret = write_pkt(pkt,in_stream,out_stream,0,meeting->output,0);
            if(ret<0){
                    av_log(NULL,AV_LOG_ERROR,"error occured while write 1 vpkt\n");
                    goto end;
            }
        }
    }
    fclose(op_f);
//}
end:
    free_meetPro(meeting);
    free(meeting);
    if (ret < 0 && ret != AVERROR_EOF & ret != AVERROR(EAGAIN)) {
        av_log(NULL,AV_LOG_ERROR, "Error occurred.\n");
        return -1;
    }
    return 0;
}