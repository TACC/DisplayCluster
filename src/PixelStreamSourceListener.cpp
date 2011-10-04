#include "PixelStreamSourceListener.h"
#include "PixelStreamSourceListenerThread.h"
#include "log.h"

PixelStreamSourceListener::PixelStreamSourceListener(int port)
{
    // assign values
    port_ = port;

    if(listen(QHostAddress::Any, port_) != true)
    {
        put_flog(LOG_FATAL, "could not listen on port %i", port_);
        exit(-1);
    }
}

void PixelStreamSourceListener::incomingConnection(int socketDescriptor)
{
    put_flog(LOG_DEBUG, "");

    PixelStreamSourceListenerThread * thread = new PixelStreamSourceListenerThread(socketDescriptor);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start();
}
