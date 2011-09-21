#ifndef MOVIE_H
#define MOVIE_H

#include <QGLWidget>

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

    private:

        void nextFrame();

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
