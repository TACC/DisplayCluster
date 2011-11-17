#ifndef NETWORK_LISTENER_H
#define NETWORK_LISTENER_H

#include <QtNetwork/QTcpServer>

class NetworkListener : public QTcpServer {
    Q_OBJECT

    public:

        NetworkListener(int port=1701);

    protected:

        void incomingConnection(int socketDescriptor);

    private:

        int port_;
};

#endif
