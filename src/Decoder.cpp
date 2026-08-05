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

#include <cmath>

// picks AV_PIX_FMT_CUDA out of the codec's offered formats so decoded frames
// stay resident on the GPU (as NV12 device memory) instead of falling back
// to a software pixel format.
//
// This can get called a second time, for the same decode, after FFmpeg has
// already tried and failed to actually initialize the CUDA hwaccel - not
// every codec's NVDEC implementation supports every resolution the codec
// itself can handle (e.g. MPEG-4 part 2 tops out around 2032px wide on
// Maxwell-generation GPUs, well under what the format allows), and that
// only surfaces at hwaccel init time, not from avcodec_get_hw_config()'s
// static capability check in _setup(). When that happens, AV_PIX_FMT_CUDA
// is no longer offered here - falling through to AV_PIX_FMT_NONE (as this
// used to do unconditionally) aborts decoding outright. Picking the first
// software format instead - same as what avcodec_default_get_format() and
// plain `ffmpeg -hwaccel cuda` do - lets decoding continue in software,
// just like it would if the codec had never advertised NVDEC support in
// the first place. ctx->opaque is set to the owning Decoder in _setup() so
// hwDecode_ can be corrected to match reality here.
static enum AVPixelFormat
get_hw_format(AVCodecContext *ctx, const enum AVPixelFormat *pix_fmts)
{
    for (const enum AVPixelFormat *p = pix_fmts; *p != AV_PIX_FMT_NONE; p++)
    {
        if (*p == AV_PIX_FMT_CUDA)
            return *p;
    }

    Decoder *decoder = (Decoder *)ctx->opaque;

    put_flog(LOG_WARN,
        "'%s': NVDEC hardware decode failed to initialize for this stream "
        "(likely a resolution beyond what this codec's NVDEC implementation "
        "supports on this GPU, even though the codec has NVDEC support in "
        "general) - falling back to SOFTWARE decode",
        decoder ? decoder->getURI().c_str() : "?");

    decoder->hwDecodeFailed();

    return pix_fmts[0];
}

Decoder::Decoder(bool paused)
{
    quit_   = false;
    pause_  = paused;
}

Decoder::~Decoder()
{
    RequestQuit();
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
    frameOffset_ = 0.;

    // an unsynchronized reading is fine here - MainWindow::updateGLWindows()
    // calls Resync() on every locally-live Decoder once per render frame
    // (see Resync() below), so this gets corrected against the shared
    // clock within one frame of the decoder thread reaching RUNNING,
    // rather than needing to coordinate a synchronized reading here itself
    tStart_ = high_resolution_clock::now();

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
    avCodecContext_->opaque = this;

    if (avcodec_parameters_to_context(avCodecContext_, stream->codecpar) < 0)
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
    // while frozen, use the target Freeze() already computed and stored
    // rather than deriving one from elapsed time - see frozenTarget_'s
    // comment in Decoder.h for why. tStart_/frameOffset_ are also written
    // from MainWindow's thread (see Resync()) once per render frame now,
    // instead of only ever being touched by this thread - lock the read,
    // same as Resync() locks the write, so neither side observes a torn
    // pair.
    int target;

    if (frozen_)
    {
        target = frozenTarget_;
    }
    else
    {
        Lock();
        auto now = high_resolution_clock::now();
        duration<double, std::ratio<1>> t = now - tStart_;
        target = int(frameOffset_ + t.count() * fps_) % (num_frames_ - 2);
        Unlock();
    }

    if (target == current_frame_)
    {
        synced_ = true;
        return true;
    }

    // mark unsynced immediately, before the catch-up loop below - which
    // can run for multiple seconds - rather than only updating synced_ at
    // its end. Otherwise a decoder that was synced right up until a new
    // gap opened (paused then resumed without being torn down, e.g.) would
    // keep reporting the old "synced" state for the whole duration of the
    // catch-up burst, telling MainWindow's cluster-wide reduction
    // everything's fine while actually scrambling to catch up - letting
    // every other tile showing this movie advance without it instead of
    // holding for it, which is exactly the per-tile straggling this
    // mechanism exists to prevent
    synced_ = false;

    // Catching up from a large gap (e.g. this decoder sat paused while its
    // content window wasn't visible on this tile - tStart_ never stops
    // ticking while paused, so the first target after being resumed can be
    // far ahead) can take more than one seek: a seek + decode-forward-to-
    // target cycle takes real time itself, so by the time it finishes,
    // target may have already moved on again. Re-check against a fresh
    // clock read after each attempt and seek again immediately rather than
    // delivering a stale frame and waiting for the next _decode() call
    // (and its scheduling/locking overhead) to notice and retry. Bounded
    // so a decoder that genuinely can't keep up (very slow storage, e.g.)
    // doesn't spin forever - it just falls back to the old one-attempt-
    // per-call behavior via the outer decode loop instead. Normal
    // steady-state playback (small target deltas, no seek needed) always
    // exits after one attempt, same as before this loop existed.
    auto catchupStart = high_resolution_clock::now();

    int attempt = 0;
    for (; ; attempt++)
    {
        int64_t dts = start_time_ + av_rescale(target, tb_.den * fr_.den, tb_.num * fr_.num);

        // only the first attempt treats "target is far ahead" as worth a
        // seek. A seek lands on the nearest keyframe <= target's dts; once
        // that's been done, later attempts in this same burst are chasing
        // a small residual gap (whatever elapsed during the previous
        // attempt's wall-clock time), and the read cursor is already
        // positioned past that keyframe - reseeking again would land on
        // the same (or a barely-later) keyframe and redecode most of what
        // was just decoded, instead of continuing forward from here. A
        // backward jump (video loop wraparound) still forces a reseek on
        // any attempt.
        if (target < current_frame_ || (attempt == 0 && (target - current_frame_) > 10))
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

                int ret = avcodec_receive_frame(avCodecContext_, avFrame_);
                if (ret == AVERROR(EAGAIN))
                {
                    // decoder needs another packet before it can produce
                    // a frame - routine (B-frame reordering, internal
                    // buffering), not an error. Feed it the next one
                    // instead of aborting the whole catch-up attempt,
                    // which previously had to restart from scratch
                    // (re-seek, re-decode-from-keyframe) on the next
                    // _decode() call - the more packets a large catch-up
                    // needs to work through, the likelier this was to
                    // hit repeatedly and never make progress at all.
                    av_packet_unref(&packet);
                    continue;
                }
                if (ret < 0)
                    return false;

                if ((avFrame_->data[0] == NULL) && (avFrame_->data[1] == NULL) && (avFrame_->data[2] == NULL))
                    continue;

                if(dts == 0 || (avFrame_->pkt_dts >= dts))
                {
                    av_packet_unref(&packet);
                    break;
                }
            }

            av_packet_unref(&packet);
        }

        if (attempt >= 4)
        {
            // still a large gap after every attempt this call is willing
            // to spend - genuinely behind, not just mid-catch-up
            synced_ = false;
            break;
        }

        int freshTarget;

        if (frozen_)
        {
            freshTarget = frozenTarget_;
        }
        else
        {
            Lock();
            auto now = high_resolution_clock::now();
            duration<double, std::ratio<1>> t = now - tStart_;
            freshTarget = int(frameOffset_ + t.count() * fps_) % (num_frames_ - 2);
            Unlock();
        }

        // small (or no) residual gap is fine to accept here - the next
        // _decode() call's normal forward-decode-without-seek path closes
        // it cheaply. Only a still-large gap, or a backward wrap, is
        // worth another seek right now.
        if (!(freshTarget < current_frame_ || (freshTarget - current_frame_) > 10))
        {
            synced_ = true;
            break;
        }

        target = freshTarget;
    }

    // logged confirmed every catch-up (not just attempt > 0 ones) takes
    // single-digit milliseconds at most here - the periodic ~1s stalls
    // being chased aren't decode/seek latency, so back to only logging
    // the genuinely-multi-attempt or slow cases, not every routine
    // one-frame advance
    duration<double, std::ratio<1>> catchupElapsed = high_resolution_clock::now() - catchupStart;

    if (attempt > 0 || catchupElapsed.count() > 0.1)
    {
        put_flog(LOG_DEBUG, "'%s' caught up to frame %d after %d extra seek(s), %.3fs",
            uri_.c_str(), current_frame_, attempt, catchupElapsed.count());
    }

    if (hwDecode_)
    {
        // avFrame_ is already an AV_PIX_FMT_CUDA frame (data[] holds
        // device pointers into GPU memory) - just share it, matching what
        // Movie.cpp's CUDA-GL interop path expects
        Lock();
        av_frame_unref(readyFrame_);
        av_frame_ref(readyFrame_, avFrame_);
        newFrame_ = true;
        Unlock();
    }
    else
    {
        // avFrame_ is in the software decoder's native format (e.g.
        // yuv422p10le for ProRes) and lives in host memory - convert to
        // NV12 so Movie.cpp only ever has to deal with one pixel layout
        // regardless of decode path, just uploaded via plain
        // glTexSubImage2D instead of the CUDA-GL interop copy
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

    return true;
}

void
Decoder::Resync(time_point<high_resolution_clock> now)
{
    Lock();
    duration<double, std::ratio<1>> t = now - tStart_;
    frameOffset_ = std::fmod(frameOffset_ + t.count() * fps_, double(num_frames_ - 2));
    tStart_ = now;
    frozen_ = false;
    Unlock();
}

void
Decoder::Freeze(time_point<high_resolution_clock> now)
{
    Lock();

    // only compute frozenTarget_ on the transition into frozen, not on
    // every call - MainWindow calls this every render frame for as long
    // as the hold lasts, and recomputing from elapsed-since-last-call each
    // time would let frozenTarget_ itself creep forward call by call
    // (exactly the drift this exists to prevent), rather than staying at
    // the single value it had the moment freezing began. Still bump
    // tStart_ forward on every call regardless, so whenever Resync() next
    // runs (unfreezing), the elapsed time it folds in is just the tail end
    // since the last Freeze() call, not the whole frozen duration.
    if (!frozen_)
    {
        duration<double, std::ratio<1>> t = now - tStart_;
        frozenTarget_ = int(frameOffset_ + t.count() * fps_) % (num_frames_ - 2);
        frozen_ = true;
    }

    tStart_ = now;

    Unlock();
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
