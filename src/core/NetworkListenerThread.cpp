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

#include <stdint.h>

NetworkListenerThread::NetworkListenerThread(int socketDescriptor)
    : socketDescriptor_(socketDescriptor)
    , tcpSocket_(new QTcpSocket(this)) // Make sure that tcpSocket_ parent is *this* so it also gets move to thread!
    , registeredToEvents_(false)
{
    if( !tcpSocket_->setSocketDescriptor(socketDescriptor_) )
    {
        put_flog(LOG_ERROR, "could not set socket descriptor: %s", tcpSocket_->errorString().toStdString().c_str());
        emit(finished());
        return;
    }

    connect(tcpSocket_, SIGNAL(disconnected()), this, SIGNAL(finished()));
    connect(tcpSocket_, SIGNAL(readyRead()), this, SLOT(process()), Qt::QueuedConnection);
    connect(this, SIGNAL(dataAvailable()), this, SLOT(process()), Qt::QueuedConnection);
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
    sendProtocolVersion();
}

void NetworkListenerThread::process()
{
    if(tcpSocket_->bytesAvailable() >= (int)MessageHeader::serializedSize)
    {
        socketReceiveMessage();
    }

    // send events if needed
    foreach (const Event& event, events_)
    {
        send(event);
    }
    events_.clear();

    // flush the socket
    tcpSocket_->flush();

    // Finish reading messages from the socket if connection closed
    if(tcpSocket_->state() != QAbstractSocket::ConnectedState)
    {
        while (tcpSocket_->bytesAvailable() >= (int)MessageHeader::serializedSize)
        {
            socketReceiveMessage();
        }
        emit(finished());
    }
    else if (tcpSocket_->bytesAvailable() >= (int)MessageHeader::serializedSize)
    {
        emit dataAvailable();
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

QByteArray NetworkListenerThread::receiveMessageBody(const int size)
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

void NetworkListenerThread::processEvent(Event event)
{
    events_.enqueue(event);
    emit dataAvailable();
}

void NetworkListenerThread::handleMessage(const MessageHeader& messageHeader, const QByteArray& byteArray)
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

    case MESSAGE_TYPE_BIND_EVENTS:
    case MESSAGE_TYPE_BIND_EVENTS_EX:
        if (registeredToEvents_)
        {
            put_flog(LOG_DEBUG, "WE are already bound!!");
        }
        else
        {
            bool eventRegistrationExclusive = (messageHeader.type == MESSAGE_TYPE_BIND_EVENTS_EX);
            emit registerToEvents(pixelStreamUri_, eventRegistrationExclusive, this);
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

void NetworkListenerThread::eventRegistrationRepy(QString uri, bool success)
{
    if (uri == pixelStreamUri_)
    {
        registeredToEvents_ = success;

        sendBindReply( registeredToEvents_ );
    }
}

void NetworkListenerThread::sendProtocolVersion()
{
    int32_t protocolVersion = NETWORK_PROTOCOL_VERSION;
    tcpSocket_->write((char *)&protocolVersion, sizeof(int32_t));

    tcpSocket_->flush();

    while(tcpSocket_->bytesToWrite() > 0)
    {
        tcpSocket_->waitForBytesWritten();
    }
}

void NetworkListenerThread::sendBindReply(const bool successful)
{
    // send message header
    MessageHeader mh(MESSAGE_TYPE_BIND_EVENTS_REPLY, sizeof(bool));
    send(mh);

    tcpSocket_->write((const char *)&successful, sizeof(bool));

    // we want the message to be sent immediately
    tcpSocket_->flush();

    while(tcpSocket_->bytesToWrite() > 0)
    {
        tcpSocket_->waitForBytesWritten();
    }
}

void NetworkListenerThread::send(const Event& event)
{
    // send message header
    MessageHeader mh(MESSAGE_TYPE_EVENT, Event::serializedSize);
    send(mh);

    {
        QDataStream stream(tcpSocket_);
        stream << event;
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
