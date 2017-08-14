/***
 h264 encoding using livavcodec

 */
#include "stddef.h" // size_t
#include <stdio.h>
#include <sys/time.h>	// gettimeofday
#include <math.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>

//#define INBUF_SIZE 4096
static AVCodec *codec;  // codect function table
static AVCodecContext *c = NULL;  // codec status
static AVFrame *frame;  // input picture
//static void *backupptr;
static AVPacket pkt;    // encoded data

#ifdef SAVE_OWN_FILE
static FILE *f;
static char *filename = "test.h264";
#endif

/*------------------------------------------------------------
 open a h264 encoder (singletone) 
 
 @TODO: multiple intances of codecs   
 -------------------------------------------------------------
 */
int ffh264_enc_open(int w, int h, int bit_rate, int fps)
{
    static int is_first = 1;
    int ret;

    // 0. init library once
    if (is_first)
    {
        avcodec_register_all();
        is_first = 0;
    }

    /* 1.1 find the video encoder */
    codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec)
    {
        fprintf(stderr, "Codec not found\n");
        return -1;
    }

    /* 1.2 create instance for codec status */
    c = avcodec_alloc_context3(codec);
    if (!c)
    {
        fprintf(stderr, "Could not allocate video codec context\n");
        return -1;
    }

    // 2.1 setting the codec paramters  

    // user level  paramters
    c->bit_rate = bit_rate;
    c->width = w;
    c->height = h;
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    //c->time_base = (AVRational){1,fps};
    c->time_base.den = fps;
    c->time_base.num = 1;
    c->gop_size = fps;   // 1? or  1 IDR/sec

    /* key for low delay operation in X264 codec */
    av_opt_set(c->priv_data, "tune", "zerolatency", 0);
    //av_opt_set(c->priv_data, "profile", "baseline", 0);
    c->delay = 0;        // low delay option 
    c->max_b_frames = 0; // no B frame  

    c->flags &= CODEC_FLAG_GLOBAL_HEADER; // SPS, PPS NAL will be included in IDR NAL 

    // quality control paramters
    c->codec_type = AVMEDIA_TYPE_VIDEO;
    c->coder_type = FF_CODER_TYPE_VLC;
    c->me_method = 7; //motion estimation algorithm
    c->me_subpel_quality = 4;
    //c->flags |= (CODEC_FLAG_QP_RD |CODEC_FLAG_LOW_DELAY  
    //           | CODEC_FLAG_QSCALE | CODEC_FLAG_EMU_EDGE);
    c->mb_decision = FF_MB_DECISION_SIMPLE;
    //c->sync_lookahead = 0;  
    //c->rc_max_rate = bit_rate;
    //c->rc_buffer_size = bit_rate * 2;

    // implementation level params
    c->thread_count = 3; // more thread more delay
    c->thread_type = FF_THREAD_SLICE;
    c->refs = 3;  // 1?

    /* 2.2 open it */
    if (avcodec_open2(c, codec, NULL) < 0)
    {
        fprintf(stderr, "Could not open codec\n");
        // @TODO free codec context before return
        return -1;
    }

    // 3. prepare frames (raw picture)
    frame = av_frame_alloc();
    if (!frame)
    {
        fprintf(stderr, "Could not allocate video frame\n");
        return -1;
    }
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;
    frame->linesize[0] = c->width;
    frame->linesize[1] = c->width / 2;
    frame->linesize[2] = c->width / 2;
    frame->pts = 0; // init pts

    //@TODO: make a dual buffer
    //  one for encoder thread, one for video input thread.
#if 1  
    /* the image (raw yuv input)
     * can be allocated by any means and av_image_alloc() is
     * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height,
            c->pix_fmt, 32);
    if (ret < 0)
    {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        return -1;
    }
#endif
    // trick heejune
    //backupptr = &frame->data[0]; 

    // compressed steam output buffer
    av_init_packet(&pkt);
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;

    return 0;
}

/*------------------------------------------------------------------
 * encode one frame 
 * return  + : compressed data output 
 0 : no compressed output
 - : error
 ------------------------------------------------------------------*/
int ffh264_enc_encode(unsigned char *pYUV, unsigned char **ppBuf)
{
    int ret, got_output;
    int width = c->width, height = c->height;

    // output buffer setting
    if (pkt.data != NULL)
    {  // previously used
        av_packet_unref(&pkt); // @TODO check, To free the allocate buffer
    }
    av_init_packet(&pkt);
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;

    //void (*cbf_save)(const unsigned char *, int) = cbf;

    //build_input_YUVframe(pYUV, frame);
    memcpy(frame->data[0], pYUV, width * height);
    memcpy(frame->data[1], pYUV + width * height, width * height / 4);
    memcpy(frame->data[2], pYUV + width * height * 5 / 4, width * height / 4);

    ++frame->pts;

    /* encode the image */
    ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
    if (ret < 0)
    {
        fprintf(stderr, "Error encoding frame\n");
        return -1;
    }
    if (got_output)
    {
        //printf("Write frame %lld (size=%5d)\n",frame->pts, pkt.size);
        *ppBuf = pkt.data;
        return pkt.size;
    }
    else
        return 0;

}

/*
 * clean up encoder  
 */
void ffh264_enc_close() //void *cbf)
{
    /* get the delayed frames */
    int i = 0, got_output, ret;
    if (pkt.data != NULL)
    {  // previously used
        av_packet_unref(&pkt); // @TODO check, To free the allocate buffer
    }

    for (got_output = 1; got_output; i++)
    {
        ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
        if (ret < 0)
        {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }
        if (got_output)
        {
            printf("Write frame %3d (size=%5d)\n", i, pkt.size);
#ifdef SAVE_OWN_FILE
            fwrite(pkt.data, 1, pkt.size, f);
#else
            //(*cbf_save)(pkt.data, pkt.size);
#endif
            av_packet_unref(&pkt);
        }
    }
    /* add sequence end code to have a real mpeg file */
    /*
     uint8_t endcode[] = { 0, 0, 1, 0xb7 };
     fwrite(endcode, 1, sizeof(endcode), f);
     */
#ifdef SAVE_OWN_FILE 
    fclose(f);
#endif

    avcodec_close(c);
    av_free(c);
    av_freep(&frame->data[0]);  // [1,2,3] is in the same allocated mem?
    //av_freep(backupptr);  // [1,2,3] is in the same allocated mem?
    av_frame_free(&frame);
    printf("\n");
}

