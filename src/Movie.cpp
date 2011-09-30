#include "Movie.h"
#include "main.h"
#include "log.h"

Movie::Movie(std::string uri)
{
    initialized_ = false;

    // defaults
    textureId_ = 0;
    textureBound_ = false;
    avFormatContext_ = NULL;
    avCodecContext_ = NULL;
    swsContext_ = NULL;
    avFrame_ = NULL;
    avFrameRGB_ = NULL;
    videoStream_ = -1;

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
    if(avcodec_open2(avCodecContext_, codec, NULL) < 0)
    {
        put_flog(LOG_ERROR, "could not open codec");
        return;
    }

    // create texture for movie
    QImage image(avCodecContext_->width, avCodecContext_->height, QImage::Format_RGB32);
    image.fill(0);

    textureId_ = g_mainWindow->getGLWindow()->bindTexture(image, GL_TEXTURE_2D, GL_RGBA, QGLContext::LinearFilteringBindOption);
    textureBound_ = true;

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

    initialized_ = true;
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
    av_close_input_file(avFormatContext_);

    // free scaler context
    sws_freeContext(swsContext_);

    // free frames
    av_free(avFrame_);
    av_free(avFrameRGB_);
}

void Movie::render(float tX, float tY, float tW, float tH)
{
    if(initialized_ != true)
    {
        return;
    }

    // advance one frame on every render
    nextFrame();

    // draw the texture
    glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId_);

    // on zoom-out, clamp to border (instead of showing the texture tiled / repeated)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

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

void Movie::nextFrame()
{
    if(initialized_ != true)
    {
        return;
    }

    int avReadStatus = 0;

    AVPacket packet;
    int frameFinished;

    while((avReadStatus = av_read_frame(avFormatContext_, &packet)) >= 0)
    {
        // make sure packet is from video stream
        if(packet.stream_index == videoStream_)
        {
            // decode video frame
            avcodec_decode_video2(avCodecContext_, avFrame_, &frameFinished, &packet);
                                     
            // make sure we got a full video frame
            if(frameFinished)
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

        // free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }

    // see if we need to loop
    if(avReadStatus < 0)
    {
        av_seek_frame(avFormatContext_, videoStream_, 0, AVSEEK_FLAG_BACKWARD);
    }
}
