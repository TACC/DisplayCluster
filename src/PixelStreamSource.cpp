#include "PixelStreamSource.h"

PixelStreamSource::PixelStreamSource(std::string uri)
{
    // defaults
    imageDataCount_ = 0;
    getImageDataCount_ = 0;

    // assign values
    uri_ = uri;
}

QByteArray PixelStreamSource::getImageData(bool & updated)
{
    QMutexLocker locker(&imageDataMutex_);

    // whether or not this is an updated image since the last call to getImageData()
    if(imageDataCount_ > getImageDataCount_)
    {
        updated = true;
    }
    else
    {
        updated = false;
    }

    getImageDataCount_ = imageDataCount_;

    return imageData_;
}

void PixelStreamSource::setImageData(QByteArray imageData)
{
    QMutexLocker locker(&imageDataMutex_);

    // only take the update if the image data has changed
    if(imageData_ != imageData)
    {
        imageData_ = imageData;
        imageDataCount_++;
    }
}

Factory<PixelStreamSource> g_pixelStreamSourceFactory;
