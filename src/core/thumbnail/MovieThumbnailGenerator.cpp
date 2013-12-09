/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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

#include "MovieThumbnailGenerator.h"

#include "Movie.h"

MovieThumbnailGenerator::MovieThumbnailGenerator(const QSize &size)
    : ThumbnailGenerator(size)
{
}

QImage MovieThumbnailGenerator::generate(const QString &filename) const
{
    Movie::initFFMPEGGlobalState();

    AVFormatContext* avFormatContext = 0;
    if( avformat_open_input( &avFormatContext, filename.toAscii(), 0, 0 ) != 0 )
        return createErrorImage("movie");

    if( avformat_find_stream_info( avFormatContext, 0 ) < 0 )
    {
        avformat_close_input(&avFormatContext);
        return createErrorImage("movie");
    }

    // find the first video stream
    int streamIdx = -1;
    for( unsigned int i = 0; i < avFormatContext->nb_streams; ++i )
    {
        if( avFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
        {
            streamIdx = i;
            break;
        }
    }

    if( streamIdx == -1 )
    {
        avformat_close_input(&avFormatContext);
        return createErrorImage("movie");
    }

    AVStream* videostream = avFormatContext->streams[streamIdx];
    AVCodecContext* avCodecContext = videostream->codec;
    AVCodec* codec = avcodec_find_decoder( avCodecContext->codec_id );
    if( !codec )
    {
        avformat_close_input(&avFormatContext);
        return createErrorImage("movie");
    }

    if( avcodec_open2( avCodecContext, codec, 0 ) < 0 )
    {
        avformat_close_input(&avFormatContext);
        return createErrorImage("movie");
    }

    AVFrame* avFrame = avcodec_alloc_frame();
    AVFrame* avFrameRGB = avcodec_alloc_frame();

    QImage image( avCodecContext->width, avCodecContext->height,
                  QImage::Format_RGB32 );
    avpicture_alloc( (AVPicture*)avFrameRGB,  PIX_FMT_RGB24,
                     image.width(), image.height( ));

    SwsContext* swsContext = sws_getContext( avCodecContext->width,
                                             avCodecContext->height,
                                             avCodecContext->pix_fmt,
                                             image.width(), image.height(),
                                             PIX_FMT_RGB24, SWS_FAST_BILINEAR,
                                             0, 0, 0 );

    // seek to 10% of movie time
    const int64_t den2 = videostream->time_base.den * videostream->r_frame_rate.den;
    const int64_t num2 = videostream->time_base.num * videostream->r_frame_rate.num;
    const int64_t num_frames = av_rescale( videostream->duration, num2, den2 );
    const int64_t desiredTimestamp = videostream->start_time +
                                       av_rescale( num_frames / 10, den2, num2 );
    if( avformat_seek_file( avFormatContext, streamIdx, 0, desiredTimestamp,
                            desiredTimestamp, 0 ) != 0 )
    {
        avcodec_close( avCodecContext );
        avformat_close_input(&avFormatContext);
        return createErrorImage("movie");
    }

    AVPacket packet;
    av_init_packet(&packet);
    while( av_read_frame( avFormatContext, &packet ) >= 0 )
    {
        if( packet.stream_index != streamIdx )
        {
            av_free_packet( &packet );
            continue;
        }

        int frameFinished;
        avcodec_decode_video2( avCodecContext, avFrame, &frameFinished,
                               &packet );

        if( !frameFinished )
        {
            av_free_packet( &packet );
            continue;
        }

        sws_scale( swsContext, avFrame->data, avFrame->linesize, 0,
                   avCodecContext->height, avFrameRGB->data,
                   avFrameRGB->linesize );

        unsigned char* src = (unsigned char*)avFrameRGB->data[0];
        for( int y = 0; y < image.height(); ++y )
        {
            QRgb* scanLine = (QRgb*)image.scanLine(y);
            for( int x = 0; x < image.width(); ++x )
                scanLine[x] = qRgb(src[3*x], src[3*x+1], src[3*x+2]);
            src += avFrameRGB->linesize[0];
        }

        av_free_packet( &packet );
        break;
    }

    avcodec_close( avCodecContext );
    avformat_close_input( &avFormatContext );
    sws_freeContext( swsContext );
    avpicture_free( (AVPicture *)avFrameRGB );
    av_free( avFrame );
    av_free( avFrameRGB );

    image = image.scaled(size_, aspectRatioMode_);
    addMetadataToImage(image, filename);
    return image;
}
