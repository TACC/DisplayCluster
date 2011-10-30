#include "PixelStreamSourceListenerThread.h"
#include "PixelStreamSource.h"
#include "PixelStreamContent.h"
#include "DisplayGroup.h" // for MessageHeader, etc.
#include "main.h"
#include "ContentWindow.h"
#include "log.h"

PixelStreamSourceListenerThread::PixelStreamSourceListenerThread(int socketDescriptor)
{
    // defaults
    initialized_ = false;

    // assign values
    socketDescriptor_ = socketDescriptor;

    // connect signals
    connect(this, SIGNAL(newImageData()), g_displayGroup.get(), SLOT(sendPixelStreams()), Qt::QueuedConnection);
}

void PixelStreamSourceListenerThread::run()
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
    while(tcpSocket.waitForReadyRead() == true)
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

        // initialize the stream if needed
        if(initialized_ != true)
        {
            uri_ = std::string(mh->uri);
            initialize();
        }

        // next, read the actual message
        QByteArray messageByteArray = tcpSocket.read(mh->size);

        while(messageByteArray.size() < mh->size)
        {
            tcpSocket.waitForReadyRead();

            messageByteArray.append(tcpSocket.read(mh->size - messageByteArray.size()));
        }

        // got the message

        // update pixel stream source
        g_pixelStreamSourceFactory.getObject(uri_)->setImageData(messageByteArray);

        // todo: later make the pixel stream source itself emit this type of signal
        emit(newImageData());
    }

    tcpSocket.disconnectFromHost();

    // note that we don't remove the Content object. the user will have to remove it explicitly.
    // the connection could have been dropped and the client might reconnect; we want to maintain
    // the window where it is on the display.
}

void PixelStreamSourceListenerThread::initialize()
{
    // add the content object if it doesn't already exist
    if(g_displayGroup->hasContent(uri_) == false)
    {
        put_flog(LOG_DEBUG, "adding pixel stream: %s", uri_.c_str());

        boost::shared_ptr<Content> c(new PixelStreamContent(uri_));
        boost::shared_ptr<ContentWindow> cw(new ContentWindow(c));

        g_displayGroup->addContentWindow(cw);
    }

    initialized_ = true;
}
