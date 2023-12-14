/*********************************************************************/
/* Copyright (c) 2011 - 2023, The University of Texas at Austin.     */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/


#include "main.h"
#include "Decoder.h"

static bool first = true;
static void sighandler(int signum)
{
    std::cerr << "Decoder kill signal\n";
    pthread_exit(NULL);
}

Decoder::Decoder(bool paused)
{
    quit_   = false;
    pause_  = paused;

    if (first)
    {
        first = false;
        signal(SIGUSR1, sighandler);
    }
}

Decoder::~Decoder()
{
    quit_ = true;
    Signal();
    pthread_join(tid_, NULL);
}

bool
Decoder::_setup()
{
    avformat_network_init();

    swsContext_ = NULL;
    avFormatContext_ = NULL;
    avCodecContext_ = NULL;
    avFrame_ = NULL;
    avFrameRGB_ = NULL;
    newFrame_ = false;

    current_frame_ = -1;
    MPI_Barrier(g_mpiRenderComm);
    tStart_ = high_resolution_clock::now();
    MPI_Barrier(g_mpiRenderComm);

    if (avformat_open_input(&avFormatContext_, uri_.c_str(), NULL, NULL) != 0)
    {
        std::cerr << "could not open movie file\n";
        return false;
    }

    if (avformat_find_stream_info(avFormatContext_, NULL) < 0)
    {
        std::cerr << "could not find stream information\n";
        return false;
    }

    videoStream_ = -1;
    AVStream *stream  = NULL;
    for (unsigned int i=0; i<avFormatContext_->nb_streams; i++)
    {
        stream = avFormatContext_->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream_ = i;
            break;
        }
    }

    if (videoStream_ == -1)
    {
        std::cerr << "could not find video stream\n";
        return false;
    }


    AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec)
    {
        std::cerr << "unsupported codec\n";
        return false;
    }

    avCodecContext_ = avcodec_alloc_context3(codec);

    if (avcodec_parameters_to_context(avCodecContext_, stream->codecpar) < 0)
    if (!codec)
    {
        std::cerr << "unable to copy codec params\n";
        return false;
    }

#if 1
    // set codec to automatically determine how many threads suits best for the decoding job
    avCodecContext_->thread_count = 0;

    if (codec->capabilities & CODEC_CAP_FRAME_THREADS)
       avCodecContext_->thread_type = FF_THREAD_FRAME;
    else if (codec->capabilities & CODEC_CAP_SLICE_THREADS)
       avCodecContext_->thread_type = FF_THREAD_SLICE;
    else
       avCodecContext_->thread_count = 1; //don't use multithreading
#endif

    if (avcodec_open2(avCodecContext_, codec, NULL) < 0)
    {
        std::cerr << "could not open codec\n";
        return false;
    }

    avFrame_ = avcodec_alloc_frame();
    avFrameRGB_ = avcodec_alloc_frame();

    numBytes_ = av_image_get_buffer_size(PIX_FMT_RGBA, avCodecContext_->width, avCodecContext_->height, 32);
    uint8_t *buffer = (uint8_t *)av_malloc(numBytes_*sizeof(uint8_t));

    av_image_fill_arrays(avFrameRGB_->data, avFrameRGB_->linesize, buffer, AV_PIX_FMT_RGBA, avCodecContext_->width, avCodecContext_->height, 1);

    duration_ = stream->duration;
    num_frames_ = av_rescale(duration_, stream->time_base.num * stream->r_frame_rate.num, stream->time_base.den * stream->r_frame_rate.den);
    start_time_ = stream->start_time;

    tb_ = stream->time_base;
    fr_ = stream->r_frame_rate;

    fps_ = (double)fr_.num / (double)fr_.den;

    height_ = avCodecContext_->height;
    width_ = avCodecContext_->width;
    data_ = &avFrameRGB_->data[0];
    linesize_ = &avFrameRGB_->linesize[0];

    swsContext_ = sws_getContext(width_, height_, avCodecContext_->pix_fmt, width_, height_, PIX_FMT_RGBA, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    return true;
}

bool 
Decoder::_decode()
{
    auto now = high_resolution_clock::now();
    duration<double, std::ratio<1>> t = now - tStart_;
    int target = int(t.count() * fps_) % (num_frames_ - 2);

    if (target == current_frame_)
        return true;

    int64_t dts = start_time_ + av_rescale(target, tb_.den * fr_.den, tb_.num * fr_.num);
    // std::cerr << "fetch " << target << "(" << current_frame_ << ", " << dts << ") at " << t.count() << " sec\n";

    if (target < current_frame_ || (target - current_frame_) > 10)
    {
        avformat_seek_file(avFormatContext_, videoStream_, 0, dts, dts, 0);
        avcodec_flush_buffers(avCodecContext_);
    }

    current_frame_ = target;

    AVPacket packet;
    while (1 == 1)
    {
        av_read_frame(avFormatContext_, &packet);

        // make sure packet is from video stream
        if(packet.stream_index == videoStream_)
        {
            // decode video frame
            
            avcodec_send_packet(avCodecContext_, &packet);

            if (avcodec_receive_frame(avCodecContext_, avFrame_))
                return false;

            if ((avFrame_->data[0] == NULL) && (avFrame_->data[1] == NULL) && (avFrame_->data[2] == NULL))
                continue;

            if(dts == 0 || (avFrame_->pkt_dts >= dts))
            {
                Lock();
                sws_scale(swsContext_, avFrame_->data, avFrame_->linesize, 0, avCodecContext_->height, avFrameRGB_->data, avFrameRGB_->linesize);
                newFrame_ = true;
                Unlock();
          
                av_packet_unref(&packet);

                break;
            }
        }

        av_packet_unref(&packet);
    }
    
    return true;
}

void
Decoder::_cleanup()
{
    avcodec_close(avCodecContext_);
    avformat_close_input(&avFormatContext_);
    sws_freeContext(swsContext_);
    av_free(avFrame_);
    av_free(avFrameRGB_);
}


