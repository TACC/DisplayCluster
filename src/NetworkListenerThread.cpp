#include "NetworkListenerThread.h"
#include "main.h"
#include "log.h"
#include "PixelStreamSource.h"

NetworkListenerThread::NetworkListenerThread(int socketDescriptor)
{
    // assign values
    socketDescriptor_ = socketDescriptor;

    // required for using MessageHeader with queued connections
    qRegisterMetaType<MessageHeader>("MessageHeader");

    // connect signals
    connect(this, SIGNAL(updatedPixelStreamSource()), g_displayGroupManager.get(), SLOT(sendPixelStreams()), Qt::QueuedConnection);
}

void NetworkListenerThread::run()
{
    QTcpSocket tcpSocket;

    if(tcpSocket.setSocketDescriptor(socketDescriptor_) != true)
    {
        put_flog(LOG_ERROR, "could not set socket descriptor: %s", tcpSocket.errorString().toStdString().c_str());
        return;
    }

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << "hello\n";
    tcpSocket.write(block);

    // read the pixel stream updates
    while(tcpSocket.waitForReadyRead() == true || tcpSocket.state() == QAbstractSocket::ConnectedState)
    {
        // first, read the message header
        QByteArray byteArray = tcpSocket.read(sizeof(MessageHeader));

        while(byteArray.size() < (int)sizeof(MessageHeader))
        {
            tcpSocket.waitForReadyRead();

            byteArray.append(tcpSocket.read(sizeof(MessageHeader) - byteArray.size()));
        }

        // got the header
        MessageHeader * mh = (MessageHeader *)byteArray.data();

        // next, read the actual message
        QByteArray messageByteArray;

        if(mh->size > 0)
        {
            messageByteArray = tcpSocket.read(mh->size);

            while(messageByteArray.size() < mh->size)
            {
                tcpSocket.waitForReadyRead();

                messageByteArray.append(tcpSocket.read(mh->size - messageByteArray.size()));
            }
        }

        // got the message
        handleMessage(*mh, messageByteArray);
    }

    tcpSocket.disconnectFromHost();

    put_flog(LOG_DEBUG, "disconnected");
}

void NetworkListenerThread::handleMessage(MessageHeader messageHeader, QByteArray byteArray)
{
    if(messageHeader.type == MESSAGE_TYPE_PIXELSTREAM)
    {
        // update pixel stream source
        // keep this in this thread so we can have pixel stream source updating and sendPixelStreams() happening in parallel
        // sendPixelStreams() slot executions may still stack up, but they'll each grab only the latest pixel stream data
        std::string uri(messageHeader.uri);

        g_pixelStreamSourceFactory.getObject(uri)->setImageData(byteArray);

        emit(updatedPixelStreamSource());
    }
    else if(messageHeader.type == MESSAGE_TYPE_PIXELSTREAM_DIMENSIONS_CHANGED)
    {
        std::string uri(messageHeader.uri);

        const int * dimensions = (const int *)byteArray.constData();

        g_pixelStreamSourceFactory.getObject(uri)->setDimensions(dimensions[0], dimensions[1]);

        emit(updatedPixelStreamSource());
    }
}
