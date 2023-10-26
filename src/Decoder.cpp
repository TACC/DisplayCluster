#include "Decoder.h"

bool
Decoder::_setup()
{
    avformat_network_init();
    av_register_all();

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
        std::cerr << "could not open movie file";
        return false;
    }

    // get stream information
    if (avformat_find_stream_info(avFormatContext_, NULL) < 0)
    {
        std::cerr << "could not find stream information";
        return false;
    }

    videoStream_ = -1;
    for (unsigned int i=0; i<avFormatContext_->nb_streams; i++)
    {
        if(avFormatContext_->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStream_ = i;
            break;
        }
    }

    if (videoStream_ == -1)
    {
        std::cerr << "could not find video stream";
        return false;
    }

    auto stream = avFormatContext_->streams[videoStream_];
    avCodecContext_ = stream->codec;

    AVCodec * codec = avcodec_find_decoder(avCodecContext_->codec_id);
    if(codec == NULL)
    {
        std::cerr << "unsupported codec";
        return false;
    }

    // set codec to automatically determine how many threads suits best for the decoding job
    avCodecContext_->thread_count = 0;

    if (codec->capabilities & CODEC_CAP_FRAME_THREADS)
       avCodecContext_->thread_type = FF_THREAD_FRAME;
    else if (codec->capabilities & CODEC_CAP_SLICE_THREADS)
       avCodecContext_->thread_type = FF_THREAD_SLICE;
    else
       avCodecContext_->thread_count = 1; //don't use multithreading

    if (avcodec_open2(avCodecContext_, codec, NULL) < 0)
    {
        std::cerr << "could not open codec\n";
        return false;
    }

    avFrame_ = avcodec_alloc_frame();
    avFrameRGB_ = avcodec_alloc_frame();

    int numBytes = avpicture_get_size(PIX_FMT_RGBA, avCodecContext_->width, avCodecContext_->height);

    uint8_t *buffer = (uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

    avpicture_fill((AVPicture *)avFrameRGB_, buffer, PIX_FMT_RGBA, avCodecContext_->width, avCodecContext_->height);

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

    int avReadStatus;
    AVPacket packet;
    int frameFinished;
    while (1 == 1)
    {
        avReadStatus = av_read_frame(avFormatContext_, &packet);

        // make sure packet is from video stream
        if(packet.stream_index == videoStream_)
        {
            // decode video frame
            avcodec_decode_video2(avCodecContext_, avFrame_, &frameFinished, &packet);
                                     
            // make sure we got a full video frame
            if(frameFinished)
            {
                // note that the last packet decoded will have a DTS corresponding to this frame's PTS
                // hence the use of avFrame_->pkt_dts as the timestamp. also, we'll keep reading frames
                // until we get to the desired timestamp (in the case that we seeked)
                if(dts == 0 || (avFrame_->pkt_dts >= dts))
                {
                    Lock();
                    // std::cerr << "d\n";
                    sws_scale(swsContext_, avFrame_->data, avFrame_->linesize, 0, avCodecContext_->height, avFrameRGB_->data, avFrameRGB_->linesize);
                    newFrame_ = true;
                    Unlock();
          

                    av_free_packet(&packet);
                    break;
                }
            }
        }

        av_free_packet(&packet);
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

