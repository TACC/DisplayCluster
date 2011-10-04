#ifndef PIXEL_STREAM_H
#define PIXEL_STREAM_H

#include <QGLWidget>
#include <QtConcurrentRun>

class PixelStream {

    public:

        PixelStream(std::string uri);
        ~PixelStream();

        void render(float tX, float tY, float tW, float tH);
        void setImageData(QByteArray imageData);
        void imageReady(QImage image);

    private:

        // pixel stream identifier
        std::string uri_;

        // texture
        GLuint textureId_;
        int textureWidth_;
        int textureHeight_;
        bool textureBound_;

        // thread for generating images from image data
        QFuture<void> loadImageDataThread_;

        // image, mutex, and ready status
        QMutex imageReadyMutex_;
        bool imageReady_;
        QImage image_;

        void setImage(QImage & image);
};

extern void loadImageDataThread(PixelStream * pixelStream, QByteArray imageData);

#endif
