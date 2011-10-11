#ifndef MOVIE_H
#define MOVIE_H

#include <QGLWidget>

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
}

class Movie {

    public:

        Movie(std::string uri);
        ~Movie();

        void render(float tX, float tY, float tW, float tH);
        void nextFrame(bool skip);

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
};

#endif
