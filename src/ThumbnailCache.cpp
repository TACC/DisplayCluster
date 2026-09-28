#include "ThumbnailCache.h"
#include "log.h"

#include <QBuffer>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QSvgRenderer>
#include <algorithm>

extern "C" {
    #include "libavcodec/avcodec.h"
    #include "libavformat/avformat.h"
    #include "libswscale/swscale.h"
}

namespace
{
    // formats whose decoder can shrink while it decodes, so huge images stay cheap
    const QList<QByteArray> SCALED_DECODE_FORMATS = { "jpeg", "jpg" };

    // other images are decoded whole, then shrunk: skip ones bigger than this
    const qint64 MAX_DECODE_PIXELS = 64LL * 1000 * 1000;

    // and even a shrinking decoder has limits
    const qint64 MAX_SCALED_DECODE_PIXELS = 1000LL * 1000 * 1000;

    // Content::getContent() decides movies by extension; so do we
    const QStringList MOVIE_SUFFIXES = { "mov", "avi", "mp4", "mkv", "mpg", "flv", "wmv" };

    // how far into a movie to take its frame - the very first is often black
    // or a title card - as a fraction of its length, capped
    const double FRAME_AT_FRACTION = 0.1;
    const double FRAME_AT_MAX_SECONDS = 10.;

    // give up on a movie that won't yield a frame within this many packets
    const int MAX_PACKETS = 2000;

    const int MAX_CONCURRENT = 2;
    const size_t MAX_ENTRIES = 500;
}

bool ThumbnailCache::possible(QString path)
{
    if(path.endsWith(".svg", Qt::CaseInsensitive) || MOVIE_SUFFIXES.contains(QFileInfo(path).suffix().toLower()))
    {
        return true;    // for a movie, whether it'll decode is only found out by trying
    }

    // Content::getContent() treats .pyr and movies by extension, not as images
    if(path.endsWith(".pyr", Qt::CaseInsensitive))
    {
        return false;
    }

    QImageReader reader(path);

    if(!reader.canRead())
    {
        return false;
    }

    QSize size = reader.size();
    qint64 pixels = (qint64)size.width() * size.height();

    if(!size.isValid())
    {
        return true;    // unknown until decoded; try
    }

    return SCALED_DECODE_FORMATS.contains(reader.format()) ? pixels <= MAX_SCALED_DECODE_PIXELS
                                                           : pixels <= MAX_DECODE_PIXELS;
}

// one frame from a movie, a little way in, no bigger than SIZE; decoded in
// software, independently of playback (Decoder), which is on the render
// processes and GPU-bound anyway
static QImage movieFrame(QString path)
{
    QImage image;
    AVFormatContext * format = NULL;
    AVCodecContext * codec = NULL;
    AVPacket * packet = NULL;
    AVFrame * frame = NULL;
    SwsContext * sws = NULL;

    std::string file = path.toStdString();

    if(avformat_open_input(&format, file.c_str(), NULL, NULL) < 0)
    {
        return image;
    }

    do
    {
        if(avformat_find_stream_info(format, NULL) < 0)
        {
            break;
        }

        const AVCodec * decoder = NULL;
        int stream = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);

        if(stream < 0 || decoder == NULL)
        {
            break;
        }

        codec = avcodec_alloc_context3(decoder);

        if(codec == NULL || avcodec_parameters_to_context(codec, format->streams[stream]->codecpar) < 0 ||
           avcodec_open2(codec, decoder, NULL) < 0)
        {
            break;
        }

        // seek a little way in; if that fails, the start will do
        if(format->duration > 0)
        {
            int64_t target = (int64_t)std::min(format->duration * FRAME_AT_FRACTION, FRAME_AT_MAX_SECONDS * AV_TIME_BASE);

            if(av_seek_frame(format, -1, target, AVSEEK_FLAG_BACKWARD) >= 0)
            {
                avcodec_flush_buffers(codec);
            }
        }

        packet = av_packet_alloc();
        frame = av_frame_alloc();

        bool got = false;

        for(int i=0; i<MAX_PACKETS && !got; i++)
        {
            int read = av_read_frame(format, packet);

            // at the end, drain the decoder
            if(read < 0)
            {
                avcodec_send_packet(codec, NULL);
            }
            else if(packet->stream_index == stream)
            {
                avcodec_send_packet(codec, packet);
            }

            if(read >= 0)
            {
                av_packet_unref(packet);
            }

            got = avcodec_receive_frame(codec, frame) == 0;

            if(read < 0 && !got)
            {
                break;
            }
        }

        if(!got || frame->width <= 0 || frame->height <= 0)
        {
            break;
        }

        QSize size = QSize(frame->width, frame->height).scaled(ThumbnailCache::SIZE, ThumbnailCache::SIZE, Qt::KeepAspectRatio);
        image = QImage(size, QImage::Format_RGB888);

        sws = sws_getContext(frame->width, frame->height, (AVPixelFormat)frame->format,
                             size.width(), size.height(), AV_PIX_FMT_RGB24, SWS_AREA, NULL, NULL, NULL);

        if(sws == NULL)
        {
            image = QImage();
            break;
        }

        uint8_t * destination[1] = { image.bits() };
        int destinationStride[1] = { (int)image.bytesPerLine() };

        sws_scale(sws, frame->data, frame->linesize, 0, frame->height, destination, destinationStride);
    }
    while(false);

    sws_freeContext(sws);
    av_frame_free(&frame);
    av_packet_free(&packet);
    avcodec_free_context(&codec);
    avformat_close_input(&format);

    return image;
}

QByteArray ThumbnailCache::make(QString path)
{
    QImage image;

    if(MOVIE_SUFFIXES.contains(QFileInfo(path).suffix().toLower()))
    {
        image = movieFrame(path);

        if(image.isNull())
        {
            put_flog(LOG_WARN, "couldn't take a thumbnail frame from %s", path.toStdString().c_str());
            return QByteArray();
        }
    }
    else if(path.endsWith(".svg", Qt::CaseInsensitive))
    {
        QSvgRenderer svg(path);

        if(!svg.isValid())
        {
            return QByteArray();
        }

        QSize size = svg.defaultSize().scaled(SIZE, SIZE, Qt::KeepAspectRatio);
        image = QImage(size.isEmpty() ? QSize(SIZE, SIZE) : size, QImage::Format_RGB32);
        image.fill(Qt::white);

        QPainter painter(&image);
        svg.render(&painter);
    }
    else
    {
        QImageReader reader(path);
        reader.setAutoTransform(true);

        QSize size = reader.size();

        // ask the decoder for (about) the final size, which the scaling
        // decoders use to skip most of the work
        if(size.isValid())
        {
            reader.setScaledSize(size.scaled(SIZE, SIZE, Qt::KeepAspectRatio));
        }

        if(!reader.read(&image))
        {
            put_flog(LOG_WARN, "couldn't make a thumbnail of %s: %s", path.toStdString().c_str(),
                     reader.errorString().toStdString().c_str());
            return QByteArray();
        }

        if(image.width() > SIZE || image.height() > SIZE)
        {
            image = image.scaled(SIZE, SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        // JPEG has no alpha: put transparent images on white, as the wall shows them
        if(image.hasAlphaChannel())
        {
            QImage opaque(image.size(), QImage::Format_RGB32);
            opaque.fill(Qt::white);
            QPainter(&opaque).drawImage(0, 0, image);
            image = opaque;
        }
    }

    QByteArray jpeg;
    QBuffer buffer(&jpeg);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "JPEG", 85);

    return jpeg;
}

void ThumbnailCache::touch(std::string uri)
{
    // caller holds mutex_
    recent_.remove(uri);
    recent_.push_front(uri);

    while(recent_.size() > MAX_ENTRIES)
    {
        entries_.erase(recent_.back());
        recent_.pop_back();
    }
}

bool ThumbnailCache::entry(std::string uri, Entry & out)
{
    QString path = QString::fromStdString(uri);
    QFileInfo info(path);

    if(!info.isFile())
    {
        return false;
    }

    QDateTime modified = info.lastModified();

    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = entries_.find(uri);

        if(it != entries_.end() && it->second.modified == modified)
        {
            touch(uri);
            out = it->second;
            return true;
        }
    }

    // a new file, or a changed one: check it (outside the lock - it reads the header)
    Entry fresh { modified, possible(path), QByteArray() };

    std::lock_guard<std::mutex> lock(mutex_);
    entries_[uri] = fresh;
    touch(uri);
    out = fresh;

    return true;
}

std::string ThumbnailCache::version(std::string uri)
{
    Entry e;

    if(!entry(uri, e) || !e.possible)
    {
        return "";
    }

    return std::to_string(e.modified.toMSecsSinceEpoch());
}

QByteArray ThumbnailCache::get(std::string uri)
{
    Entry e;

    if(!entry(uri, e) || !e.possible)
    {
        return QByteArray();
    }

    if(!e.jpeg.isEmpty())
    {
        return e.jpeg;
    }

    // wait for a free slot, then make it - unless someone else did meanwhile
    {
        std::unique_lock<std::mutex> lock(mutex_);
        slotFree_.wait(lock, [this]() { return making_ < MAX_CONCURRENT; });

        auto it = entries_.find(uri);

        if(it != entries_.end() && it->second.modified == e.modified && !it->second.jpeg.isEmpty())
        {
            return it->second.jpeg;
        }

        making_++;
    }

    QByteArray jpeg = make(QString::fromStdString(uri));

    std::lock_guard<std::mutex> lock(mutex_);
    making_--;
    slotFree_.notify_one();

    auto it = entries_.find(uri);

    if(it != entries_.end() && it->second.modified == e.modified)
    {
        if(jpeg.isEmpty())
        {
            it->second.possible = false;    // don't keep trying a file that won't decode
        }
        else
        {
            it->second.jpeg = jpeg;
        }
    }

    return jpeg;
}
