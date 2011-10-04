#ifndef PIXEL_STREAM_SOURCE_LISTENER_THREAD_H
#define PIXEL_STREAM_SOURCE_LISTENER_THREAD_H

#include <QThread>
#include <QtNetwork/QTcpSocket>

class PixelStreamSourceListenerThread : public QThread {
    Q_OBJECT

    public:

        PixelStreamSourceListenerThread(int socketDescriptor);

        void run();

    signals:

        void newImageData();

    private:

        void initialize();

        bool initialized_;
        std::string uri_;
        int socketDescriptor_;
};

#endif
