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
#include "globals.h"
#include "DisplayGroupManager.h"
#include "MainWindow.h"
#include "GLWindow.h"
#include "log.h"

Movie::Movie(QString uri)
    : uri_(uri)
    , textureId_(0)
    // FFMPEG
    , avFormatContext_(NULL)
    , avCodecContext_(NULL)
    , swsContext_(NULL)
    , avFrame_(NULL)
    , avFrameRGB_(NULL)
    , streamIdx_(-1)
    , videostream_(NULL)
    // Internal
    , start_time_(0)
    , duration_(0)
    , num_frames_(0)
    , frameDuration_(0)
    , frame_index_(0)
    , skipped_frames_(false)
    , paused_(false)
    , loop_(true)
{
    Movie::initFFMPEGGlobalState();

    // open movie file
    if(avformat_open_input(&avFormatContext_, uri.toAscii(), NULL, NULL) != 0)
    {
        put_flog(LOG_ERROR, "could not open movie file %s", uri.toLocal8Bit().constData());
        return;
    }

    // get stream information
    if(avformat_find_stream_info(avFormatContext_, NULL) < 0)
    {
        put_flog(LOG_ERROR, "could not find stream information");
        return;
    }

    // dump format information to stderr
    av_dump_format(avFormatContext_, 0, uri.toAscii(), 0);

    // find the first video stream
    streamIdx_ = -1;

    for(unsigned int i=0; i<avFormatContext_->nb_streams; i++)
    {
        if(avFormatContext_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            streamIdx_ = i;
            break;
        }
    }

    if(streamIdx_ == -1)
    {
        put_flog(LOG_ERROR, "could not find video stream");
        return;
    }

    videostream_ = avFormatContext_->streams[streamIdx_];

    // get a pointer to the codec context for the video stream
    avCodecContext_ = videostream_->codec;

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

    // allocate video frame for video decoding
    avFrame_ = avcodec_alloc_frame();

    // allocate video frame for RGB conversion
    avFrameRGB_ = avcodec_alloc_frame();

    if( !avFrame_ || !avFrameRGB_ )
    {
        put_flog(LOG_ERROR, "error allocating frames");
        return;
    }

    den2_ = videostream_->time_base.den * videostream_->r_frame_rate.den;
    num2_ = videostream_->time_base.num * videostream_->r_frame_rate.num;

    // generate seeking parameters
    start_time_ = videostream_->start_time;
    duration_ = videostream_->duration;
    num_frames_ = av_rescale(duration_, num2_, den2_);
    frameDuration_ = (double)videostream_->r_frame_rate.den / (double)videostream_->r_frame_rate.num;

    put_flog(LOG_DEBUG, "seeking parameters: start_time = %i, duration_ = %i, num frames = %i", start_time_, duration_, num_frames_);

    // create texture for movie
    QImage image(avCodecContext_->width, avCodecContext_->height, QImage::Format_RGB32);
    image.fill(0);

    textureId_ = g_mainWindow->getGLWindow()->bindTexture(image, GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // assign buffer to pFrameRGB
    avpicture_alloc( (AVPicture *)avFrameRGB_, PIX_FMT_RGBA, avCodecContext_->width, avCodecContext_->height);

    // create sws scaler context
    swsContext_ = sws_getContext(avCodecContext_->width, avCodecContext_->height, avCodecContext_->pix_fmt, avCodecContext_->width, avCodecContext_->height, PIX_FMT_RGBA, SWS_FAST_BILINEAR, NULL, NULL, NULL);
}

Movie::~Movie()
{
    if(textureId_)
        g_mainWindow->getGLWindow()->deleteTexture(textureId_);

    avcodec_close( avCodecContext_ );

    // close the format context
    avformat_close_input(&avFormatContext_);

    // free scaler context
    sws_freeContext(swsContext_);

    // free frames
    avpicture_free( (AVPicture *)avFrameRGB_ );
    av_free(avFrame_);
    av_free(avFrameRGB_);
}

void Movie::initFFMPEGGlobalState()
{
    static bool initialized = false;

    if (!initialized)
    {
        av_register_all();
        initialized = true;
    }
}

void Movie::getDimensions(int &width, int &height)
{
    width = avCodecContext_->width;
    height = avCodecContext_->height;
}

void Movie::render(float tX, float tY, float tW, float tH)
{
    updateRenderedFrameIndex();

    if(!textureId_)
        return;

    // draw the texture
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId_);

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
    if( !textureId_ || (paused_ && !skipped_frames_) )
        return;

    // rate limiting
    double elapsedSeconds = 999999.;
    if( !nextTimestamp_.is_not_a_date_time( ))
        elapsedSeconds = (g_displayGroupManager->getTimestamp() -
                           nextTimestamp_).total_microseconds() / 1000000.;

    if( elapsedSeconds < frameDuration_ )
        return;

    // update timestamp of last frame
    nextTimestamp_ = g_displayGroupManager->getTimestamp();

    // seeking

    // where we should be after we read a frame in this function
    if( !paused_ )
        ++frame_index_;

    // if we're skipping this frame, increment the skipped frame counter and return
    // otherwise, make sure we're in the correct position in the stream and decode
    if( skip )
    {
        skipped_frames_ = true;
        return;
    }

    // keep track of a desired timestamp if we seek in this frame
    int64_t desiredTimestamp = 0;
    if( skipped_frames_ )
    {
        // need to seek

        // frame number we want
        const int64_t index = (frame_index_-1) % (num_frames_+1);

        // timestamp we want
        desiredTimestamp = start_time_ + av_rescale(index, den2_, num2_);

        // seek to the nearest keyframe before desiredTimestamp and flush buffers
        if(avformat_seek_file(avFormatContext_, streamIdx_, 0, desiredTimestamp, desiredTimestamp, 0) != 0)
        {
            put_flog(LOG_ERROR, "seeking error");
            return;
        }

        avcodec_flush_buffers(avCodecContext_);

        skipped_frames_ = false;
    }

    int avReadStatus = 0;

    AVPacket packet;
    av_init_packet(&packet);
    int frameFinished;

    while((avReadStatus = av_read_frame(avFormatContext_, &packet)) >= 0)
    {
        // make sure packet is from video stream
        if(packet.stream_index == streamIdx_)
        {
            // decode video frame
            avcodec_decode_video2(avCodecContext_, avFrame_, &frameFinished, &packet);

            // make sure we got a full video frame
            if(frameFinished)
            {
                // note that the last packet decoded will have a DTS corresponding to this frame's PTS
                // hence the use of avFrame_->pkt_dts as the timestamp. also, we'll keep reading frames
                // until we get to the desired timestamp (in the case that we seeked)
                if(desiredTimestamp == 0 || (avFrame_->pkt_dts >= desiredTimestamp))
                {
                    // convert the frame from its native format to RGB
                    sws_scale(swsContext_, avFrame_->data, avFrame_->linesize, 0, avCodecContext_->height, avFrameRGB_->data, avFrameRGB_->linesize);

                    // put the RGB image to the already-created texture
                    // glTexSubImage2D uses the existing texture and is more efficient than other means
                    glBindTexture(GL_TEXTURE_2D, textureId_);
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0,0, avCodecContext_->width, avCodecContext_->height, GL_RGBA, GL_UNSIGNED_BYTE, avFrameRGB_->data[0]);

                    // free the packet that was allocated by av_read_frame
                    av_free_packet(&packet);

                    break;
                }
            }
        }

        // free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    // see if we need to loop
    if(avReadStatus < 0 && loop_)
    {
        av_seek_frame(avFormatContext_, streamIdx_, 0, AVSEEK_FLAG_BACKWARD);
        frame_index_ = 0;
    }
}

void Movie::setPause(const bool pause)
{
    paused_ = pause;
}

void Movie::setLoop(const bool loop)
{
    loop_ = loop;
}
