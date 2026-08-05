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
#include <atomic>

using namespace std::chrono;

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libavutil/error.h"
    #include "libavutil/mathematics.h"
    #include "libavutil/avutil.h"
    #include "libavutil/imgutils.h"
    #include "libavutil/hwcontext.h"
    #include "libswscale/swscale.h"
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
            while (decoder->pause_  && !decoder->quit_)
                decoder->Wait();
            decoder->Unlock();

            if (decoder->quit_)
                break;

            decoder->_decode();

            // _decode()'s fast path (nothing new to do) returns almost
            // instantly, and without this, an idle decoder spins as fast
            // as the CPU allows, forever - harmless with one or two movies
            // open, but with several each burning a full core continuously,
            // this starves the main thread and every other decoder thread
            // of scheduling time, which (among other things) undermines
            // Freeze()'s ability to hold a target steady, since that
            // depends on the main thread getting scheduled promptly. 1ms
            // bounds each decoder to on the order of 1000 checks/sec - far
            // more than the ~24-60 times/sec actually needed to keep up
            // with video frame boundaries, so this costs nothing
            // perceptible while cutting idle CPU usage by orders of
            // magnitude.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
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

    // fire-and-forget, not synchronous: the decoder thread's loop polls
    // pause_ on every iteration and will notice it went false on its own
    // very shortly regardless. This used to Wait() for the decoder thread
    // to Signal() back, but that only worked if the decoder thread was
    // actually blocked in its own Wait() (i.e. had already observed
    // pause_==true) at the moment Resume() ran - a Pause() immediately
    // followed by a Resume(), faster than the decoder thread's loop gets
    // back around to checking pause_, meant the decoder thread saw
    // pause_==false right away, skipped its own Wait()/Signal() entirely,
    // and left this Wait() blocked on a Signal() that was never coming.
    // Nothing here actually needs Resume() to block until the decoder
    // thread confirms it woke up, so there's nothing to lose by not
    // waiting for it.
    void Resume()
    {
        Lock();
        pause_ = false;
        Signal();
        Unlock();
    }

    // signals the decoder thread to quit without waiting for it to
    // actually exit (unlike ~Decoder(), which does this then joins).
    // Exists so a caller tearing down several Decoders at once - see
    // GLWindow::finalize() - can signal every one of them first and only
    // then join them, instead of destroying them one at a time via
    // Factory::clear() and blocking on each thread_.join() before even
    // signaling the next decoder to quit. A decoder mid-catch-up can take
    // real time (up to a few seconds) to notice quit_ and return from its
    // current _decode() call; joining sequentially means that wait is
    // paid once per movie, added up, rather than once, in parallel,
    // across all of them.
    void RequestQuit()
    {
        Lock();
        quit_ = true;
        Unlock();
        Signal();
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

    // true once _decode() has landed within a small gap of its target
    // frame (or needed no catch-up at all); false while genuinely behind.
    // Written by the decoder thread inside _decode(), read by MainWindow's
    // thread every render frame as part of deciding whether every rank
    // showing this movie is caught up - atomic rather than behind mutex_
    // since it's a single flag checked far more often than it changes and
    // isn't part of any multi-field invariant the way tStart_/frameOffset_
    // are (see Resync())
    bool
    isSynced() { return synced_; }

    int
    getNumberOfFrames() { return num_frames_; };

    // false means this codec has no NVDEC hwaccel support and _decode() is
    // falling back to software decode + an sws_scale() conversion to NV12
    // (see _decode()) - Movie::render() needs to know this to pick between
    // the CUDA-GL interop upload path and a plain glTexSubImage2D one
    bool
    usingHardwareDecode() { return hwDecode_; }

    // called from get_hw_format() (via AVCodecContext::opaque) when the
    // codec advertised NVDEC support but the hwaccel failed to actually
    // initialize for this particular stream - see its comment for why that
    // can happen despite the avcodec_get_hw_config() check in _setup()
    // passing. Corrects hwDecode_ to match the software fallback FFmpeg is
    // actually using, so _decode()/Movie::render() pick the matching path
    void
    hwDecodeFailed() { hwDecode_ = false; }

    // re-anchors tStart_ to a fresh clock reading supplied by the caller,
    // absorbing the frame count already elapsed into frameOffset_ so this
    // doesn't cause a visible jump (see MainWindow::updateGLWindows(),
    // which calls this on every locally-live Decoder once per render
    // frame using one shared reading, rather than each Decoder deciding
    // for itself when and how to resync - a Decoder never touches MPI).
    void Resync(time_point<high_resolution_clock> now);

    // like Resync(), but does NOT fold the elapsed time into frameOffset_
    // first - it just pins tStart_ to now, so the target frame this
    // decoder computes stays constant instead of advancing. Called every
    // render frame in place of Resync() while MainWindow's cluster-wide
    // reduction says some rank showing this movie isn't caught up yet: a
    // straggler chasing a target that keeps moving in real time even
    // though the whole display is frozen for viewers might never converge
    // if its decode throughput can't beat real-time; freezing the target
    // itself guarantees it will, given any positive throughput at all.
    // frameOffset_ being untouched here is what makes the transition back
    // to Resync() once caught up seamless - only a frame or so of elapsed
    // time to fold in, since tStart_ was just pinned a moment ago.
    void Freeze(time_point<high_resolution_clock> now);

private:

    bool _setup();
    bool _decode();
    void _cleanup();

    std::string uri_;
    int width_, height_;
    int current_frame_ = -1;
    bool newFrame_;
    bool hwDecode_ = true;
    SwsContext *swsContext_ = nullptr;
    // starts false, not true: a freshly-created decoder hasn't run
    // _decode() even once yet, and defaulting to "synced" would let the
    // cluster-wide reduction in MainWindow::updateGLWindows() report
    // everyone caught up while this tile hasn't produced a single real
    // frame - only _decode() itself, having actually confirmed the gap is
    // small (or closed it), gets to claim synced
    std::atomic<bool> synced_{false};

    // while frozen_, _decode() uses frozenTarget_ directly instead of
    // computing target from elapsed time since tStart_ - see Freeze().
    // Earlier this was done by just re-pinning tStart_ to "now" on every
    // call without a separate stored target, which only kept the
    // computed target truly constant if Freeze() was called often enough
    // (roughly more than fps_/10 times a second) - under load, with
    // several decoder threads and the main thread contending for CPU, the
    // gap between calls could exceed that, and _decode()'s own
    // continuously-running elapsed-time computation would see the target
    // creep forward each time regardless of how promptly it got re-pinned.
    // An explicit stored value has no such timing dependency: it only
    // changes when Freeze() is explicitly called again.
    std::atomic<bool> frozen_{false};
    std::atomic<int> frozenTarget_{0};

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

    // frame count already elapsed as of tStart_ - lets Resync() re-anchor
    // tStart_ to a fresh clock reading without the current frame visibly
    // jumping. A double, not an int: Resync() runs every render frame now
    // (not just every 300s), so the elapsed time it folds in each call is
    // often well under one video frame's duration - truncating that to an
    // int on every call would discard the fractional progress every time
    // and playback would never accumulate past frame 0. Only the final
    // target frame index (see _decode()) gets truncated to int.
    double frameOffset_ = 0.;
};

#endif
