#include "Decoder.h"

Decoder::Decoder(bool paused)
{
    quit_   = false;
    pause_  = paused;
}

Decoder::~Decoder()
{
    quit_ = true;
    Signal();
    pthread_join(tid_, NULL);
}

bool
Decoder::_setup()
{
    avformat_network_init();
    // av_register_all();

    swsContext_ = NULL;
    avFormatContext_ = NULL;
    avCodecContext_ = NULL;
    avFrame_ = NULL;
    avFrameRGB_ = NULL;
    newFrame_ = false;

    current_frame_ = -1;
    tStart_ = high_resolution_clock::now();

    if (avformat_open_input(&avFormatContext_, uri_.c_str(), NULL, NULL) != 0)
    {
        std::cerr << "could not open movie file\n";
        return false;
    }

    if (avformat_find_stream_info(avFormatContext_, NULL) < 0)
    {
        std::cerr << "could not find stream information\n";
        return false;
    }

    videoStream_ = -1;
    AVStream *stream  = NULL;
    for (unsigned int i=0; i<avFormatContext_->nb_streams; i++)
    {
        stream = avFormatContext_->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream_ = i;
            break;
        }
    }

    if (videoStream_ == -1)
    {
        std::cerr << "could not find video stream\n";
        return false;
    }


    AVCodec *codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec)
    {
        std::cerr << "unsupported codec\n";
        return false;
    }

    avCodecContext_ = avcodec_alloc_context3(codec);

    if (avcodec_parameters_to_context(avCodecContext_, stream->codecpar) < 0)
    if (!codec)
    {
        std::cerr << "unable to copy codec params\n";
        return false;
    }

#if 1
    // set codec to automatically determine how many threads suits best for the decoding job
    avCodecContext_->thread_count = 0;

    if (codec->capabilities & CODEC_CAP_FRAME_THREADS)
       avCodecContext_->thread_type = FF_THREAD_FRAME;
    else if (codec->capabilities & CODEC_CAP_SLICE_THREADS)
       avCodecContext_->thread_type = FF_THREAD_SLICE;
    else
       avCodecContext_->thread_count = 1; //don't use multithreading
#endif

    if (avcodec_open2(avCodecContext_, codec, NULL) < 0)
    {
        std::cerr << "could not open codec\n";
        return false;
    }

    avFrame_ = avcodec_alloc_frame();
    avFrameRGB_ = avcodec_alloc_frame();

    int numBytes = av_image_get_buffer_size(PIX_FMT_RGBA, avCodecContext_->width, avCodecContext_->height, 32);

    uint8_t *buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    av_image_fill_arrays(avFrameRGB_->data, avFrameRGB_->linesize, buffer, AV_PIX_FMT_RGBA, avCodecContext_->width, avCodecContext_->height, 1);

    duration_ = stream->duration;
    num_frames_ = av_rescale(duration_, stream->time_base.num * stream->r_frame_rate.num, stream->time_base.den * stream->r_frame_rate.den);
    start_time_ = stream->start_time;

    tb_ = stream->time_base;
    fr_ = stream->r_frame_rate;

    fps_ = (double)fr_.num / (double)fr_.den;

    height_ = avCodecContext_->height;
    width_ = avCodecContext_->width;
    data_ = &avFrameRGB_->data[0];
    linesize_ = &avFrameRGB_->linesize[0];

    swsContext_ = sws_getContext(width_, height_, avCodecContext_->pix_fmt, width_, height_, PIX_FMT_RGBA, SWS_FAST_BILINEAR, NULL, NULL, NULL);

    return true;
}

bool 
Decoder::_decode()
{
    auto now = high_resolution_clock::now();
    duration<double, std::ratio<1>> t = now - tStart_;
    int target = int(t.count() * fps_) % (num_frames_ - 2);

    if (target == current_frame_)
        return true;

    int64_t dts = start_time_ + av_rescale(target, tb_.den * fr_.den, tb_.num * fr_.num);
    // std::cerr << "fetch " << target << "(" << current_frame_ << ", " << dts << ") at " << t.count() << " sec\n";

    if (target < current_frame_ || (target - current_frame_) > 10)
    {
        avformat_seek_file(avFormatContext_, videoStream_, 0, dts, dts, 0);
        avcodec_flush_buffers(avCodecContext_);
    }

    current_frame_ = target;

    AVPacket packet;
    while (1 == 1)
    {
        av_read_frame(avFormatContext_, &packet);

        // make sure packet is from video stream
        if(packet.stream_index == videoStream_)
        {
            // decode video frame
            
            avcodec_send_packet(avCodecContext_, &packet);

            if (avcodec_receive_frame(avCodecContext_, avFrame_))
                return false;

            if ((avFrame_->data[0] == NULL) && (avFrame_->data[1] == NULL) && (avFrame_->data[2] == NULL))
                continue;

            if(dts == 0 || (avFrame_->pkt_dts >= dts))
            {
                Lock();
                sws_scale(swsContext_, avFrame_->data, avFrame_->linesize, 0, avCodecContext_->height, avFrameRGB_->data, avFrameRGB_->linesize);
                newFrame_ = true;
                Unlock();
          
                av_packet_unref(&packet);

                break;
            }
        }

        av_packet_unref(&packet);
    }
    
    return true;
}

void
Decoder::_cleanup()
{
    avformat_close_input(&avFormatContext_);
    sws_freeContext(swsContext_);
    av_free(avFrame_);
    av_free(avFrameRGB_);
}


