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
#include <pthread.h>

#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

using namespace std::chrono;

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"
    #include "libavutil/error.h"
    #include "libavutil/mathematics.h"
    #include "libavutil/avutil.h"
    #include "libavutil/imgutils.h"
}

#if LIBAVUTIL_VERSION_MAJOR == 56
#define PIX_FMT_RGBA AV_PIX_FMT_RGBA
#define avcodec_alloc_frame av_frame_alloc
#define CODEC_CAP_FRAME_THREADS AV_CODEC_CAP_FRAME_THREADS
#define CODEC_CAP_SLICE_THREADS AV_CODEC_CAP_SLICE_THREADS
#endif

class Decoder 
{
private:
    static void *
    decoderThread(void *p)
    {
        Decoder *decoder = (Decoder *)p;
        // std::cerr << "Decoder thread pid: " << ((long)gettid()) << "\n";
       
        decoder->Lock();
        if (decoder->_setup())
          decoder->tState_ = RUNNING;
        else
          decoder->tState_ = ERROR;
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

        // std::cerr << "Decoder thread exit\n";
        pthread_exit(0);
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

        pthread_mutex_init(&lock_, NULL);
        pthread_cond_init(&signal_, NULL);

        tState_ = START;

        Lock();

        if (pthread_create(&tid_, NULL, decoderThread, (void *)this))
        {
            Unlock();
            return false;
        }

        while (tState_ == START)
            Wait();
        Unlock();

        return tState_ == RUNNING;
    }

    void Lock()   { pthread_mutex_lock(&lock_); }
    void Unlock() { pthread_mutex_unlock(&lock_); }
    void Signal() { pthread_cond_signal(&signal_); }
    void Kill()   { pthread_kill(tid_, SIGUSR1); }
    void Wait()   { pthread_cond_wait(&signal_, &lock_); }

    void
    getFrameDimensions(int &w, int &h)
    {
        w = width_;
        h = height_;
    }

    uint8_t *
    getFrame()
    {
        Lock();
        newFrame_ = false;
        return data_[0];
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
    uint8_t **data_;
    int *linesize_;
    int current_frame_ = -1;
    bool newFrame_;

    SwsContext * swsContext_;
    AVFormatContext *avFormatContext_;
    AVCodecContext *avCodecContext_; 
    AVFrame *avFrame_, *avFrameRGB_;
    int64_t duration_, num_frames_, start_time_; 
    int videoStream_;
    double fps_;
    AVRational tb_;
    AVRational fr_;

    enum ThreadState { START, RUNNING, ERROR } tState_ = START;

    pthread_t tid_;
    bool quit_, pause_;

    pthread_mutex_t lock_;
    pthread_cond_t  signal_;

    time_point<high_resolution_clock> tStart_;
};

#endif
