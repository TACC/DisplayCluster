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

#ifndef MOVIE_H
#define MOVIE_H

#include "FactoryObject.h"
#include <QGLWidget>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <chrono>
#include <iostream>
using namespace std::chrono;


// required for FFMPEG includes below, specifically for the Linux build
#ifdef __cplusplus
    #ifndef __STDC_CONSTANT_MACROS
        #define __STDC_CONSTANT_MACROS
    #endif

    #ifdef _STDINT_H
        #undef _STDINT_H
    #endif

    #include <stdint.h>
#endif

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswscale/swscale.h>
    #include <libavutil/error.h>
    #include <libavutil/mathematics.h>
}

#if LIBAVCODEC_VERSION_INT > AV_VERSION_INT(55,28,1)
#define avcodec_alloc_frame av_frame_alloc
#define avcodec_free_frame av_frame_free
#define PIX_FMT_RGBA AV_PIX_FMT_RGBA
#endif

class Movie : public FactoryObject {

    public:

        Movie(std::string uri);
        ~Movie();

        void getDimensions(int &width, int &height);
        void render(float tX, float tY, float tW, float tH);
        void nextFrame(bool);

    private:

        // true if all the movie initializations were successful
        bool initialized_;

        // image location
        std::string uri_;

        // texture
        GLuint textureId_;
        bool textureBound_;

        // FFMPEG
        AVFormatContext * avFormatContext_;
        AVCodecContext * avCodecContext_; // this is a member of AVFormatContext, saved for convenience; no need to free
        SwsContext * swsContext_;
        AVFrame * avFrame_;
        AVFrame * avFrameRGB_;
        int videoStream_;

        // used for seeking
        int64_t start_time_;
        int64_t duration_;
        int64_t num_frames_;

        int64_t frame_index_;
        int64_t skipped_frames_;

		double FPS_;

        // frame timing
		time_point<high_resolution_clock> tFirst;
		time_point<high_resolution_clock> tStart;
		int64_t decode_count_;
};

#endif
