#ifndef PIXEL_STREAM_SOURCE_H
#define PIXEL_STREAM_SOURCE_H

#include "Factory.hpp"
#include <QtGui>

class PixelStreamSource {

    public:

        PixelStreamSource(std::string uri);

        QByteArray getImageData(bool & updated);
        void setImageData(QByteArray imageData);

        void getDimensions(int &width, int &height, bool & updated);
        void setDimensions(int width, int height);

    private:

        // pixel stream source identifier
        std::string uri_;

        // image data, mutex for accessing it, and counter for updates
        QMutex imageDataMutex_;
        QByteArray imageData_;
        long imageDataCount_;

        // imageDataCount of last retrieval via getImageData()
        long getImageDataCount_;

        // dimensions, mutex for accessing it, and counter for updates
        QMutex dimensionsMutex_;
        int width_;
        int height_;
        long dimensionsCount_;

        // dimensionsCount of last retrieval via getDimensions()
        long getDimensionsCount_;
};

// global pixel stream source factory
extern Factory<PixelStreamSource> g_pixelStreamSourceFactory;

#endif
