/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
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

#include "Movie.h"
#include "main.h"
#include "log.h"

#include <chrono>
#include <iostream>
using namespace std::chrono;


Movie::Movie(std::string uri)
{
    tCreation = high_resolution_clock::now();

    std::cerr << "movie ctor " << uri << "\n";
    initialized_ = false;
    decode_count_ = 0;

    // defaults
    textureId_ = 0;
    textureBound_ = false;
    avFormatContext_ = NULL;
    avCodecContext_ = NULL;
    swsContext_ = NULL;
    avFrame_ = NULL;
    avFrameRGB_ = NULL;
    videoStream_ = -1;

    start_time_ = 0;
    duration_ = 0;
    num_frames_ = 0;
    last_frame_index_ = -2;
    skipped_frames_ = 0;

    // assign values
    uri_ = uri;

    // initialize ffmpeg
    av_register_all();

    // open movie file
    if(avformat_open_input(&avFormatContext_, uri.c_str(), NULL, NULL) != 0)
    {
        put_flog(LOG_ERROR, "could not open movie file");
        return;
    }

    // get stream information
    if(avformat_find_stream_info(avFormatContext_, NULL) < 0)
    {
        put_flog(LOG_ERROR, "could not find stream information");
        return;
    }

    // dump format information to stderr
    av_dump_format(avFormatContext_, 0, uri.c_str(), 0);

    // find the first video stream
    videoStream_ = -1;

    for(unsigned int i=0; i<avFormatContext_->nb_streams; i++)
    {
        if(avFormatContext_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream_ = i;
            break;
        }
    }

    if(videoStream_ == -1)
    {
        put_flog(LOG_ERROR, "could not find video stream");
        return;
    }

    // get a pointer to the codec context for the video stream
    avCodecContext_ = avFormatContext_->streams[videoStream_]->codec;

    // find the decoder for the video stream
    AVCodec * codec = avcodec_find_decoder(avCodecContext_->codec_id);

    if(codec == NULL)
    {
        put_flog(LOG_ERROR, "unsupported codec");
        return;
    }

    // open codec
    int ret = avcodec_open2(avCodecContext_, codec, NULL);

    if(ret < 0)
    {
        char errbuf[256];
        av_strerror(ret, errbuf, 256);

        put_flog(LOG_ERROR, "could not open codec, error code %i: %s", ret, errbuf);
        return;
    }

    // generate seeking parameters
    auto stream = avFormatContext_->streams[videoStream_];

    start_time_ = stream->start_time;
    duration_ = stream->duration;
    num_frames_ = av_rescale(duration_, stream->time_base.num * stream->r_frame_rate.num, stream->time_base.den * stream->r_frame_rate.den);
    FPS_ = (double)stream->r_frame_rate.num / (double)stream->r_frame_rate.den;

    put_flog(LOG_DEBUG, "seeking parameters: start_time = %i, duration_ = %i, num frames = %i", start_time_, duration_, num_frames_);

    // allocate video frame for video decoding
    avFrame_ = avcodec_alloc_frame();

    // allocate video frame for RGB conversion
    avFrameRGB_ = avcodec_alloc_frame();

    if(avFrame_ == NULL || avFrameRGB_ == NULL)
    {
        put_flog(LOG_ERROR, "error allocating frames");
        return;
    }

    // get required buffer size and allocate buffer for pFrameRGB
    // this memory will be overwritten during frame conversion, but needs to be allocated ahead of time
    int numBytes = avpicture_get_size(PIX_FMT_RGBA, avCodecContext_->width, avCodecContext_->height);
    uint8_t * buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    // assign buffer to pFrameRGB
    avpicture_fill((AVPicture *)avFrameRGB_, buffer, PIX_FMT_RGBA, avCodecContext_->width, avCodecContext_->height);

    // create sws scaler context
    swsContext_ = sws_getContext(avCodecContext_->width, avCodecContext_->height, avCodecContext_->pix_fmt, avCodecContext_->width, avCodecContext_->height, PIX_FMT_RGBA, SWS_FAST_BILINEAR, NULL, NULL, NULL);

}

Movie::~Movie()
{
    if(textureBound_ == true)
    {
        // delete bound texture
        glDeleteTextures(1, &textureId_); // it appears deleteTexture() below is not actually deleting the texture from the GPU...
        g_mainWindow->getGLWindow()->deleteTexture(textureId_);
    }

    // close the format context
    avformat_close_input(&avFormatContext_);

    // free scaler context
    sws_freeContext(swsContext_);

    // free frames
    av_free(avFrame_);
    av_free(avFrameRGB_);
}

void Movie::getDimensions(int &width, int &height)
{
    width = avCodecContext_->width;
    height = avCodecContext_->height;
}

void Movie::render(float tX, float tY, float tW, float tH)
{
    updateRenderedFrameCount();

    // draw the texture

    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);
    glEnable(GL_TEXTURE_2D);

    if(initialized_ != true)
    {
        std::cerr << "GL VERSION: " << glGetString(GL_VERSION) << " " << glGetString(GL_VENDOR) << "\n";

        glClearColor(0.0, 0.0, 0.0, 0.0);
        glShadeModel(GL_FLAT);
        glEnable(GL_DEPTH_TEST);

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glGenTextures(1, &textureId_);
        glBindTexture(GL_TEXTURE_2D, textureId_);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, avCodecContext_->width, avCodecContext_->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

        initialized_ = true;
    }
    else
        glBindTexture(GL_TEXTURE_2D, textureId_);

    glBindTexture(GL_TEXTURE_2D, textureId_);

    // on zoom-out, clamp to edge (instead of showing the texture tiled / repeated)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBegin(GL_QUADS);

    glTexCoord2f(tX,tY);
    glVertex2f(0.,0.);

    glTexCoord2f(tX+tW,tY);
    glVertex2f(1.,0.);

    glTexCoord2f(tX+tW,tY+tH);
    glVertex2f(1.,1.);

    glTexCoord2f(tX,tY+tH);
    glVertex2f(0.,1.);

    glEnd();

    glPopAttrib();
}

void Movie::nextFrame(bool skip)
{
    auto now = high_resolution_clock::now();
    duration<double, std::ratio<1>> tInterval = now - tOut;

    if (last_frame_index_ == -2)
    {
        last_frame_index_ = -1;
        tTiming = now;
    }

    duration<double, std::ratio<1>> tElapsed = now - tCreation;
    int calculated_frame_index_ = (int)(tElapsed.count() * FPS_);
    auto stream = avFormatContext_->streams[videoStream_];

    if (last_frame_index_ > calculated_frame_index_)
    {
        tElapsed = duration<double, std::ratio<1>>::zero();
        tCreation = now;
        calculated_frame_index_ = 0;
        last_frame_index_ = -1;
    }

    if (last_frame_index_ == calculated_frame_index_)
        return;

    // std::cerr << "l: " << last_frame_index_ << "  c: " << calculated_frame_index_ << "\n";


    // int64_t dts = start_time_ + av_rescale(tElapsed.count(), stream->time_base.den, stream->time_base.num);
    int64_t dts = (int64_t)((tElapsed.count() * stream->time_base.den) /  stream->time_base.num);


    // The number in the next statement is a guestimate about the relative cost of decoding extra vs seeking.
    if ((last_frame_index_ < 0) || (last_frame_index_ < calculated_frame_index_ - 99999) || (last_frame_index_ > calculated_frame_index_))
    {
#if 0
        if (last_frame_index_ == -1)
            std::cerr << g_mpiRank << ": rewind at " << tElapsed.count() << "\n";
        else if (last_frame_index_ < calculated_frame_index_ - 5) 
            std::cerr << g_mpiRank << ": catch up " << (calculated_frame_index_ - last_frame_index_) << "\n";
        else 
            std::cerr << g_mpiRank << ": seek back... why? " << last_frame_index_ << " " << calculated_frame_index_ << "\n";
#endif
        avformat_seek_file(avFormatContext_, videoStream_, 0, dts, dts, 0);
        avcodec_flush_buffers(avCodecContext_);
    }

    last_frame_index_  = calculated_frame_index_;

    AVPacket packet;
    int frameFinished;
    int avr;

    for (bool done = false; !done; )
    {
        avr = av_read_frame(avFormatContext_, &packet);
        if (avr < 0)
        {
            calculated_frame_index_ = num_frames_;
            last_frame_index_ = -1;
            tCreation = now;
            done = true;
        }
        else if (packet.stream_index == videoStream_)
        {
            // decode video frame
            avcodec_decode_video2(avCodecContext_, avFrame_, &frameFinished, &packet);

            // make sure we got a full video frame
            if(frameFinished)
            {
                // note that the last packet decoded will have a DTS corresponding to this frame's PTS
                // hence the use of avFrame_->pkt_dts as the timestamp. also, we'll keep reading frames
                // until we get to the desired timestamp (in the case that we seeked)
                if((avr == AVERROR_EOF) || (dts == 0) || (avFrame_->pkt_dts >= dts))
                {
                    // convert the frame from its native format to RGB and copy it to the texture
                    sws_scale(swsContext_, avFrame_->data, avFrame_->linesize, 0, avCodecContext_->height, avFrameRGB_->data, avFrameRGB_->linesize);
                    glBindTexture(GL_TEXTURE_2D, textureId_);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, avCodecContext_->width, avCodecContext_->height, GL_RGBA, GL_UNSIGNED_BYTE, avFrameRGB_->data[0]);
                    done = true;
                }
            }
        }

        // free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    if (++decode_count_ % 100 == 0)
    {
        duration<double, std::ratio<1>> tTot = now - tTiming;
        if (g_dc_flags & 0x1)
            std::cerr << decode_count_ << " " << (decode_count_ / tTot.count()) << " FPS Reset\n";
        decode_count_ = 0;
        tTiming = now;
    }

    tOut = high_resolution_clock::now();
}
