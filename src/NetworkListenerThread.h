#ifndef NETWORK_LISTENER_THREAD_H
#define NETWORK_LISTENER_THREAD_H

// increment this every time the network protocol changes in a major way
#define NETWORK_PROTOCOL_VERSION 1

#include "DisplayGroupManager.h"
#include <QThread>
#include <QtNetwork/QTcpSocket>

class NetworkListenerThread : public QThread {
    Q_OBJECT

    public:

        NetworkListenerThread(int socketDescriptor);

        void run();

    signals:

        void updatedPixelStreamSource();

    private:

        int socketDescriptor_;

        void handleMessage(MessageHeader messageHeader, QByteArray byteArray);
};

#endif
