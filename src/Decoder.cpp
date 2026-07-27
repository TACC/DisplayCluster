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
#include "log.h"

namespace
{
    // how often (in seconds) each decoder re-anchors its frame-timing
    // epoch (tStart_) via a fresh barrier-synchronized clock reading.
    // _setup() only does this once, at startup; std::chrono::
    // high_resolution_clock on different hosts doesn't run at exactly the
    // same rate (NTP disciplines it, but not perfectly, and it can be
    // adjusted between resyncs), so "elapsed = now - tStart_" slowly
    // diverges between hosts the longer tStart_ stays fixed - on a
    // long-running installation, that drift is unbounded without a
    // periodic re-sync. 300s (5 min) bounds it to whatever a single
    // interval's worth of clock-rate error amounts to, which should be
    // imperceptible for any reasonable clock. Override via
    // DISPLAYCLUSTER_MOVIE_RESYNC_SEC; 0 disables periodic resync
    // entirely, matching the original one-time-only behavior.
    double
    decoderResyncIntervalSec()
    {
        static const double interval = (getenv("DISPLAYCLUSTER_MOVIE_RESYNC_SEC") == NULL) ? 300. : atof(getenv("DISPLAYCLUSTER_MOVIE_RESYNC_SEC"));
        return interval;
    }
}

// picks AV_PIX_FMT_CUDA out of the codec's offered formats so decoded frames
// stay resident on the GPU (as NV12 device memory) instead of falling back
// to a software pixel format
static enum AVPixelFormat
get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    for (const enum AVPixelFormat *p = pix_fmts; *p != AV_PIX_FMT_NONE; p++)
    {
        if (*p == AV_PIX_FMT_CUDA)
            return *p;
    }

    std::cerr << "failed to get CUDA hw surface format; codec has no NVDEC support\n";
    return AV_PIX_FMT_NONE;
}

Decoder::Decoder(bool paused)
{
    quit_   = false;
    pause_  = paused;
}

Decoder::~Decoder()
{
    quit_ = true;
    Signal();
    thread_.join();
}

bool
Decoder::_setup()
{
    avformat_network_init();

    avFormatContext_ = NULL;
    avCodecContext_ = NULL;
    avFrame_ = NULL;
    readyFrame_ = NULL;
    hwDeviceCtx_ = NULL;
    newFrame_ = false;

    current_frame_ = -1;
    frameOffset_ = 0;
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


    const AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
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

    // not every codec has an NVDEC hwaccel variant at all (ProRes, for
    // one, never will - NVIDIA has never implemented it, on any GPU
    // generation) - check before committing to the hw path, rather than
    // discovering it later via a get_format() callback failure
    hwDecode_ = false;
    for (int i = 0; ; i++)
    {
        const AVCodecHWConfig *config = avcodec_get_hw_config(codec, i);
        if (!config)
            break;
        if ((config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX) &&
            config->device_type == AV_HWDEVICE_TYPE_CUDA)
        {
            hwDecode_ = true;
            break;
        }
    }

    if (hwDecode_)
    {
        if (av_hwdevice_ctx_create(&hwDeviceCtx_, AV_HWDEVICE_TYPE_CUDA, NULL, NULL, 0) < 0)
        {
            std::cerr << "could not create CUDA hw device context\n";
            return false;
        }

        avCodecContext_->hw_device_ctx = av_buffer_ref(hwDeviceCtx_);
        avCodecContext_->get_format = get_hw_format;
    }
    else
    {
        put_flog(LOG_WARN,
            "'%s' is %s, which has no NVDEC hardware decode support on any NVIDIA GPU - "
            "falling back to SOFTWARE decode. This is dramatically slower and will likely "
            "struggle to keep up at cluster-wall resolutions/frame rates, especially with "
            "multiple movies playing at once. You will get much better results by "
            "transcoding this file to H.264 or HEVC first, e.g.:  "
            "ffmpeg -i \"%s\" -c:v libx264 -crf 18 -pix_fmt yuv420p output.mp4",
            uri_.c_str(), codec->name, uri_.c_str());
    }

    if (avcodec_open2(avCodecContext_, codec, NULL) < 0)
    {
        std::cerr << "could not open codec\n";
        return false;
    }

    avFrame_ = avcodec_alloc_frame();
    readyFrame_ = avcodec_alloc_frame();

    duration_ = stream->duration;
    num_frames_ = av_rescale(duration_, stream->time_base.num * stream->r_frame_rate.num, stream->time_base.den * stream->r_frame_rate.den);
    start_time_ = stream->start_time;

    tb_ = stream->time_base;
    fr_ = stream->r_frame_rate;

    fps_ = (double)fr_.num / (double)fr_.den;

    height_ = avCodecContext_->height;
    width_ = avCodecContext_->width;

    return true;
}

bool
Decoder::_decode()
{
    auto now = high_resolution_clock::now();
    duration<double, std::ratio<1>> t = now - tStart_;

    // periodically re-anchor tStart_ via a fresh barrier-synchronized
    // clock reading to bound cross-host drift over a long-running
    // session - see decoderResyncIntervalSec() above. frameOffset_
    // absorbs the frame count already elapsed so this doesn't cause a
    // visible jump: right after the resync, frameOffset_ + 0 elapsed
    // still equals whatever frame we were already on.
    double resyncIntervalSec = decoderResyncIntervalSec();
    if (resyncIntervalSec > 0. && t.count() >= resyncIntervalSec)
    {
        frameOffset_ = (frameOffset_ + int(t.count() * fps_)) % (num_frames_ - 2);

        MPI_Barrier(g_mpiRenderComm);
        now = high_resolution_clock::now();
        MPI_Barrier(g_mpiRenderComm);

        tStart_ = now;
        t = duration<double, std::ratio<1>>::zero();
    }

    int target = (frameOffset_ + int(t.count() * fps_)) % (num_frames_ - 2);

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
                if (hwDecode_)
                {
                    // avFrame_ is already an AV_PIX_FMT_CUDA frame (data[]
                    // holds device pointers into GPU memory) - just share
                    // it, matching what Movie.cpp's CUDA-GL interop path
                    // expects
                    Lock();
                    av_frame_unref(readyFrame_);
                    av_frame_ref(readyFrame_, avFrame_);
                    newFrame_ = true;
                    Unlock();
                }
                else
                {
                    // avFrame_ is in the software decoder's native format
                    // (e.g. yuv422p10le for ProRes) and lives in host
                    // memory - convert to NV12 so Movie.cpp only ever has
                    // to deal with one pixel layout regardless of decode
                    // path, just uploaded via plain glTexSubImage2D
                    // instead of the CUDA-GL interop copy
                    if (!swsContext_)
                    {
                        swsContext_ = sws_getContext(
                            avFrame_->width, avFrame_->height, (AVPixelFormat)avFrame_->format,
                            avFrame_->width, avFrame_->height, AV_PIX_FMT_NV12,
                            SWS_BILINEAR, NULL, NULL, NULL);
                    }

                    Lock();
                    av_frame_unref(readyFrame_);
                    readyFrame_->format = AV_PIX_FMT_NV12;
                    readyFrame_->width = avFrame_->width;
                    readyFrame_->height = avFrame_->height;
                    av_frame_get_buffer(readyFrame_, 0);
                    sws_scale(swsContext_, avFrame_->data, avFrame_->linesize, 0, avFrame_->height,
                              readyFrame_->data, readyFrame_->linesize);
                    newFrame_ = true;
                    Unlock();
                }

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
    // avcodec_close() was deprecated years ago and is gone entirely from
    // newer FFmpeg headers (still present in the older FFmpeg this builds
    // against on Linux, which is why this didn't surface until the Windows
    // build against a newer FFmpeg); avcodec_free_context() closes and
    // frees in one call
    avcodec_free_context(&avCodecContext_);
    avformat_close_input(&avFormatContext_);
    av_frame_free(&avFrame_);
    av_frame_free(&readyFrame_);
    av_buffer_unref(&hwDeviceCtx_);
    sws_freeContext(swsContext_);
    swsContext_ = nullptr;
}
