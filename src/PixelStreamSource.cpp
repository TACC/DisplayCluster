#include "PixelStreamSource.h"

PixelStreamSource::PixelStreamSource(std::string uri)
{
    // defaults
    imageDataCount_ = 0;
    getImageDataCount_ = 0;

    dimensionsCount_ = 0;
    getDimensionsCount_ = 0;

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

void PixelStreamSource::getDimensions(int &width, int &height, bool & updated)
{
    QMutexLocker locker(&dimensionsMutex_);

    // whether or not these are updated dimensions since the last call to getDimensions()
    if(dimensionsCount_ > getDimensionsCount_)
    {
        updated = true;
    }
    else
    {
        updated = false;
    }

    getDimensionsCount_ = dimensionsCount_;

    width = width_;
    height = height_;
}

void PixelStreamSource::setDimensions(int width, int height)
{
    QMutexLocker locker(&dimensionsMutex_);

    // only take the update if the dimensions have changed
    if(width != width_ || height != height_)
    {
        width_ = width;
        height_ = height;

        dimensionsCount_++;
    }
}

Factory<PixelStreamSource> g_pixelStreamSourceFactory;
