#include "NetworkListenerThread.h"
#include "main.h"
#include "log.h"

NetworkListenerThread::NetworkListenerThread(int socketDescriptor)
{
    // assign values
    socketDescriptor_ = socketDescriptor;

    // required for using MessageHeader with queued connections
    qRegisterMetaType<MessageHeader>("MessageHeader");

    // connect signals
    connect(this, SIGNAL(newMessage(MessageHeader, QByteArray)), g_displayGroup.get(), SLOT(handleMessage(MessageHeader, QByteArray)), Qt::QueuedConnection);
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
        QByteArray messageByteArray = tcpSocket.read(mh->size);

        while(messageByteArray.size() < mh->size)
        {
            tcpSocket.waitForReadyRead();

            messageByteArray.append(tcpSocket.read(mh->size - messageByteArray.size()));
        }

        // got the message
        emit(newMessage(*mh, messageByteArray));
    }

    tcpSocket.disconnectFromHost();

    put_flog(LOG_DEBUG, "disconnected");
}
