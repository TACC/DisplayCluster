#ifndef PIXEL_STREAM_SOURCE_LISTENER_H
#define PIXEL_STREAM_SOURCE_LISTENER_H

#include <QtNetwork/QTcpServer>

class PixelStreamSourceListener : public QTcpServer {
    Q_OBJECT

    public:

        PixelStreamSourceListener(int port=1701);

    protected:

        void incomingConnection(int socketDescriptor);

    private:

        int port_;
};

#endif
