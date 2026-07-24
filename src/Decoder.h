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

#ifndef DECODER_H
#define DECODER_H

#include <iostream>
#include <stdint.h>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std::chrono;

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/error.h"
    #include "libavutil/mathematics.h"
    #include "libavutil/avutil.h"
    #include "libavutil/imgutils.h"
    #include "libavutil/hwcontext.h"
}

// these old names have been retired since FFmpeg ~3.x; alias them for any
// reasonably modern FFmpeg rather than pinning to one exact libavutil release
#if LIBAVUTIL_VERSION_MAJOR >= 56
#define PIX_FMT_RGBA AV_PIX_FMT_RGBA
#define avcodec_alloc_frame av_frame_alloc
#define CODEC_CAP_FRAME_THREADS AV_CODEC_CAP_FRAME_THREADS
#define CODEC_CAP_SLICE_THREADS AV_CODEC_CAP_SLICE_THREADS
#endif

class Decoder
{
private:
    static void
    decoderThread(Decoder *decoder)
    {
        decoder->Lock();
        if (decoder->_setup())
          decoder->tState_ = RUNNING;
        else
          decoder->tState_ = DECODE_FAILED;
        decoder->Signal();
        decoder->Unlock();

        while (! decoder->quit_)
        {
            decoder->Lock();
            if (decoder->pause_)
            {
                while (decoder->pause_  && !decoder->quit_)
                    decoder->Wait();
                decoder->Signal();
            }
            decoder->Unlock();

            if (decoder->quit_)
                break;

            decoder->_decode();
        }

        decoder->_cleanup();
    }

public:
    Decoder(bool paused = false);
    ~Decoder();

    std::string 
    getURI() { return uri_; }

    bool isPaused() { return pause_; }

    void Pause()
    { 
        Lock();
        pause_ = true;
        Unlock();
    }

    void Resume()
    {
        Lock();
        pause_ = false;
        Signal();
        Wait();
        Unlock();
    }

    bool
    Setup(std::string uri)
    {
        uri_   = uri;

        tState_ = START;

        Lock();

        thread_ = std::thread(decoderThread, this);

        while (tState_ == START)
            Wait();
        Unlock();

        return tState_ == RUNNING;
    }

    void Lock()   { mutex_.lock(); }
    void Unlock() { mutex_.unlock(); }
    void Signal() { cv_.notify_one(); }

    // mirrors pthread_cond_wait(): caller must already hold mutex_ (via a
    // prior Lock()); adopt_lock takes over that ownership without
    // re-locking (a fresh lock() attempt here would throw, since a
    // unique_lock refuses to re-lock a mutex it doesn't already own itself,
    // and this one doesn't - Lock()/Unlock() touch mutex_ directly, not a
    // unique_lock). release() stops the temporary from unlocking on scope
    // exit, since cv_.wait() re-locks before returning and the caller is
    // still on the hook for the matching Unlock()
    void
    Wait()
    {
        std::unique_lock<std::mutex> ul(mutex_, std::adopt_lock);
        cv_.wait(ul);
        ul.release();
    }

    void
    getFrameDimensions(int &w, int &h)
    {
        w = width_;
        h = height_;
    }

    // hardware (NVDEC) frame ready for GPU-side consumption; caller must
    // call releaseFrame() once done reading planes/linesize from it, since
    // the decoder thread is locked out until then
    AVFrame *
    getFrame()
    {
        Lock();
        newFrame_ = false;
        return readyFrame_;
    }

    void
    releaseFrame()
    {
        Unlock();
    }

    bool
    ready() { return newFrame_; }

    int
    getNumberOfFrames() { return num_frames_; };

private:

    bool _setup();
    bool _decode();
    void _cleanup();

    std::string uri_;
    int width_, height_;
    int current_frame_ = -1;
    bool newFrame_;

    AVFormatContext *avFormatContext_;
    AVCodecContext *avCodecContext_;
    AVFrame *avFrame_, *readyFrame_;
    AVBufferRef *hwDeviceCtx_;
    int64_t duration_, num_frames_, start_time_;
    int videoStream_;
    double fps_;
    AVRational tb_;
    AVRational fr_;

    // "ERROR" would collide with the ERROR macro <windows.h> defines
    // (same family of landmine as min/max, which is why NOMINMAX is set)
    enum ThreadState { START, RUNNING, DECODE_FAILED } tState_ = START;

    std::thread thread_;
    bool quit_, pause_;

    std::mutex mutex_;
    std::condition_variable cv_;

    time_point<high_resolution_clock> tStart_;

    // frame count already elapsed as of tStart_ - lets periodic re-syncs
    // (see decoderResyncIntervalSec() in Decoder.cpp) re-anchor tStart_
    // to a fresh barrier-synchronized clock reading without the current
    // frame visibly jumping
    int frameOffset_ = 0;
};

#endif
