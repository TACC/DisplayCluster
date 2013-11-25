/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#include "NetworkListenerThread.h"

// increment this every time the network protocol changes in a major way
#include "NetworkProtocol.h"
#include "PixelStream.h"
#include "log.h"

#include "ContentWindowManager.h"

#include "DisplayGroupManager.h"
#include "PixelStreamDispatcher.h"

#include <stdint.h>

NetworkListenerThread::NetworkListenerThread(PixelStreamDispatcher &pixelStreamDispatcher, DisplayGroupManager &displayGroupManager, int socketDescriptor)
    : socketDescriptor_(socketDescriptor)
    , tcpSocket_(0)
    , interactionBound_(false)
    , pixelStreamDispatcher_(pixelStreamDispatcher)
    , displayGroupManager_(displayGroupManager)
{
}

NetworkListenerThread::~NetworkListenerThread()
{
    put_flog(LOG_DEBUG, "");

    if( tcpSocket_->state() == QAbstractSocket::ConnectedState )
        sendQuit();

    delete tcpSocket_;
}

void NetworkListenerThread::initialize()
{
    tcpSocket_ = new QTcpSocket();

    if(tcpSocket_->setSocketDescriptor(socketDescriptor_) != true)
    {
        put_flog(LOG_ERROR, "could not set socket descriptor: %s", tcpSocket_->errorString().toStdString().c_str());
        emit(finished());
        return;
    }

    // Make local connections
    connect(tcpSocket_, SIGNAL(disconnected()), this, SIGNAL(finished()));

    // DisplayGroupManager
    connect(&displayGroupManager_, SIGNAL(pixelStreamViewClosed(QString)), this, SLOT(pixelStreamerClosed(QString)));

    // PixelStreamDispatcher
    connect(this, SIGNAL(receivedAddPixelStreamSource(QString,int)), &pixelStreamDispatcher_, SLOT(addSource(QString,int)));
    connect(this, SIGNAL(receivedPixelStreamSegement(QString,int,PixelStreamSegment)), &pixelStreamDispatcher_, SLOT(processSegment(QString,int,PixelStreamSegment)));
    connect(this, SIGNAL(receivedPixelStreamFinishFrame(QString,int)), &pixelStreamDispatcher_, SLOT(processFrameFinished(QString,int)));
    connect(this, SIGNAL(receivedRemovePixelStreamSource(QString,int)), &pixelStreamDispatcher_, SLOT(removeSource(QString,int)));

    // get a local DisplayGroupInterface to help manage interaction
    bool success = QMetaObject::invokeMethod(&displayGroupManager_, "getDisplayGroupInterface", Qt::BlockingQueuedConnection, Q_RETURN_ARG(boost::shared_ptr<DisplayGroupInterface>, displayGroupInterface_), Q_ARG(QThread *, QThread::currentThread()));

    if(success != true)
    {
        put_flog(LOG_ERROR, "error getting DisplayGroupInterface");
        emit(finished());
        return;
    }

    // todo: we need to consider the performance of the low delay option
    // tcpSocket_->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // handshake
    int32_t protocolVersion = NETWORK_PROTOCOL_VERSION;
    tcpSocket_->write((char *)&protocolVersion, sizeof(int32_t));

    tcpSocket_->flush();

    while(tcpSocket_->bytesToWrite() > 0)
    {
        tcpSocket_->waitForBytesWritten();
    }

    // start a timer-based event loop...
    QTimer * timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(process()));
    timer->start(1);
}

void NetworkListenerThread::process()
{
    // receive a message if available
    tcpSocket_->waitForReadyRead(1);

    if(tcpSocket_->bytesAvailable() >= (int)sizeof(MessageHeader))
    {
        socketReceiveMessage();
    }

    // send events if needed
    foreach (const InteractionState& interactionState, interactionStates_)
    {
        send(interactionState);
    }
    interactionStates_.clear();

    // flush the socket
    tcpSocket_->flush();

    // Finish reading messages from the socket if connection closed
    if(tcpSocket_->state() != QAbstractSocket::ConnectedState)
    {
        while (tcpSocket_->bytesAvailable() >= (int)sizeof(MessageHeader))
        {
            socketReceiveMessage();
        }
        emit(finished());
    }
}

void NetworkListenerThread::socketReceiveMessage()
{
    // first, read the message header
    MessageHeader mh = receiveMessageHeader();

    // next, read the actual message
    QByteArray messageByteArray = receiveMessageBody(mh.size);

    // got the message
    handleMessage(mh, messageByteArray);
}

MessageHeader NetworkListenerThread::receiveMessageHeader()
{    
    MessageHeader messageHeader;

    QDataStream stream(tcpSocket_);
    stream >> messageHeader;

    return messageHeader;
}

QByteArray NetworkListenerThread::receiveMessageBody(int size)
{
    // next, read the actual message
    QByteArray messageByteArray;

    if(size > 0)
    {
        messageByteArray = tcpSocket_->read(size);

        while(messageByteArray.size() < size)
        {
            tcpSocket_->waitForReadyRead();

            messageByteArray.append(tcpSocket_->read(size - messageByteArray.size()));
        }
    }

    return messageByteArray;
}

void NetworkListenerThread::setInteractionState(InteractionState interactionState)
{
    interactionStates_.enqueue(interactionState);
}

void NetworkListenerThread::handleMessage(MessageHeader messageHeader, QByteArray byteArray)
{
    QString uri(messageHeader.uri);

    switch(messageHeader.type)
    {
    case MESSAGE_TYPE_QUIT:
        if (pixelStreamUri_ == uri)
        {
            emit receivedRemovePixelStreamSource(uri, socketDescriptor_);
            pixelStreamUri_ = QString();
        }
        break;

    case MESSAGE_TYPE_PIXELSTREAM_OPEN:
        if (pixelStreamUri_.isEmpty())
        {
            pixelStreamUri_ = uri;
            emit receivedAddPixelStreamSource(uri, socketDescriptor_);
        }
        break;

    case MESSAGE_TYPE_PIXELSTREAM_FINISH_FRAME:
        if (pixelStreamUri_ == uri)
        {
            emit receivedPixelStreamFinishFrame(uri, socketDescriptor_);
        }
        break;

    case MESSAGE_TYPE_PIXELSTREAM:
        handlePixelStreamMessage(uri, byteArray);
        break;

    case MESSAGE_TYPE_BIND_INTERACTION:
    case MESSAGE_TYPE_BIND_INTERACTION_EX:
        if (interactionBound_)
        {
            put_flog(LOG_DEBUG, "WE are already bound!!");
        }
        else
        {
            interactionName_ = uri;
            interactionExclusive_ = (messageHeader.type == MESSAGE_TYPE_BIND_INTERACTION_EX);
            interactionBound_ = bindInteraction();
            sendBindReply( interactionBound_ );
        }
        break;

    default:
        break;
    }

}

void NetworkListenerThread::handlePixelStreamMessage(const QString& uri, const QByteArray& byteArray)
{
    const PixelStreamSegmentParameters* parameters = (const PixelStreamSegmentParameters *)(byteArray.data());

    PixelStreamSegment segment;
    segment.parameters = *parameters;

    // read image data
    QByteArray imageData = byteArray.right(byteArray.size() - sizeof(PixelStreamSegmentParameters));
    segment.imageData = imageData;

    if (pixelStreamUri_ == uri)
    {
        emit(receivedPixelStreamSegement(uri, socketDescriptor_, segment));
    }
    else
    {
        put_flog(LOG_INFO, "received PixelStreamSegement from incorrect uri: %s", uri.toLocal8Bit().constData());
    }
}

void NetworkListenerThread::pixelStreamerClosed(QString uri)
{
    if (uri == pixelStreamUri_)
    {
        emit(finished());
    }
}

bool NetworkListenerThread::bindInteraction()
{
    // try to bind to the ContentWindowManager corresponding to interactionName
    ContentWindowManagerPtr cwm = displayGroupInterface_->getContentWindowManager(interactionName_);

    if(cwm)
    {
        put_flog(LOG_DEBUG, "found window: '%s'", interactionName_.toStdString().c_str());

        QMutexLocker locker( cwm->getInteractionBindMutex( ));

        // if an interaction is already bound, don't bind this one if exclusive
        // was requested
        if( cwm->isInteractionBound() && interactionExclusive_ )
        {
            return false;
        }

        // todo: disconnect any existing signal connections to the setInteractionState() slot
        // in case we're binding to another window in the same connection / socket

        // make connection to get interaction updates
        cwm->bindInteraction( this, SLOT(setInteractionState(InteractionState)));
        return true;
    }
    else
    {
        put_flog(LOG_DEBUG, "could not find window: '%s'", interactionName_.toStdString().c_str());
        return false;
    }
}

void NetworkListenerThread::sendBindReply( bool successful )
{
    // send message header
    MessageHeader mh(MESSAGE_TYPE_BIND_INTERACTION_REPLY, sizeof(bool));
    send(mh);

    tcpSocket_->write((const char *)&successful, sizeof(bool));

    // we want the message to be sent immediately
    tcpSocket_->flush();

    while(tcpSocket_->bytesToWrite() > 0)
    {
        tcpSocket_->waitForBytesWritten();
    }

    interactionName_.clear();
}

void NetworkListenerThread::send(const InteractionState& interactionState)
{
    // send message header
    MessageHeader mh(MESSAGE_TYPE_INTERACTION, sizeof(InteractionState));
    send(mh);

    // send interaction state
    int sent = tcpSocket_->write((const char *)&interactionState, sizeof(InteractionState));

    while(sent < (int)sizeof(InteractionState))
    {
        sent += tcpSocket_->write((const char *)&interactionState + sent, sizeof(InteractionState) - sent);
    }

    // we want the message to be sent immediately
    tcpSocket_->flush();

    while(tcpSocket_->bytesToWrite() > 0)
    {
        tcpSocket_->waitForBytesWritten();
    }
}

void NetworkListenerThread::sendQuit()
{
    MessageHeader mh(MESSAGE_TYPE_QUIT, 0);
    send(mh);

    // we want the message to be sent immediately
    tcpSocket_->flush();

    while(tcpSocket_->bytesToWrite() > 0)
    {
        tcpSocket_->waitForBytesWritten();
    }
}

bool NetworkListenerThread::send(const MessageHeader& messageHeader)
{
    QDataStream stream(tcpSocket_);
    stream << messageHeader;

    return stream.status() == QDataStream::Ok;
}
