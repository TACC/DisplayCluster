#include "NetworkListener.h"
#include "NetworkListenerThread.h"
#include "log.h"

NetworkListener::NetworkListener(int port)
{
    // assign values
    port_ = port;

    if(listen(QHostAddress::Any, port_) != true)
    {
        put_flog(LOG_FATAL, "could not listen on port %i", port_);
        exit(-1);
    }
}

void NetworkListener::incomingConnection(int socketDescriptor)
{
    put_flog(LOG_DEBUG, "");

    NetworkListenerThread * thread = new NetworkListenerThread(socketDescriptor);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}
