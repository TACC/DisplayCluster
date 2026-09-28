#include "ThumbnailCache.h"
#include "log.h"

#include <QBuffer>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QSvgRenderer>

namespace
{
    // formats whose decoder can shrink while it decodes, so huge images stay cheap
    const QList<QByteArray> SCALED_DECODE_FORMATS = { "jpeg", "jpg" };

    // other images are decoded whole, then shrunk: skip ones bigger than this
    const qint64 MAX_DECODE_PIXELS = 64LL * 1000 * 1000;

    // and even a shrinking decoder has limits
    const qint64 MAX_SCALED_DECODE_PIXELS = 1000LL * 1000 * 1000;

    const int MAX_CONCURRENT = 2;
    const size_t MAX_ENTRIES = 500;
}

bool ThumbnailCache::possible(QString path)
{
    if(path.endsWith(".svg", Qt::CaseInsensitive))
    {
        return true;
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

QByteArray ThumbnailCache::make(QString path)
{
    QImage image;

    if(path.endsWith(".svg", Qt::CaseInsensitive))
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
