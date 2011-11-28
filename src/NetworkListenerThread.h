#ifndef NETWORK_LISTENER_THREAD_H
#define NETWORK_LISTENER_THREAD_H

#include "DisplayGroupManager.h"
#include <QThread>
#include <QtNetwork/QTcpSocket>

class NetworkListenerThread : public QThread {
    Q_OBJECT

    public:

        NetworkListenerThread(int socketDescriptor);

        void run();

    signals:

        void newMessage(MessageHeader messageHeader, QByteArray byteArray);

    private:

        int socketDescriptor_;
};

#endif
