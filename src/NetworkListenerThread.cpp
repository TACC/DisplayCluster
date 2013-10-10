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
#include "globals.h"
#include "log.h"
#include "PixelStream.h"
#include "SVGStreamSource.h"
#include "ContentWindowManager.h"
#include <stdint.h>

NetworkListenerThread::NetworkListenerThread(int socketDescriptor)
{
    // defaults
    tcpSocket_ = NULL;
    interactionBound_ = false;
    updatedInteractionState_ = false;

    // assign values
    socketDescriptor_ = socketDescriptor;
}

NetworkListenerThread::~NetworkListenerThread()
{
    put_flog(LOG_DEBUG, "");

    if(tcpSocket_ != NULL)
    {
        delete tcpSocket_;
    }
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

    // make connections
    connect(tcpSocket_, SIGNAL(disconnected()), this, SIGNAL(finished()));
    connect(this, SIGNAL(updatedSVGStreamSource()), g_displayGroupManager.get(), SLOT(sendSVGStreams()), Qt::BlockingQueuedConnection);

    connect(this, SIGNAL(receivedOpenPixelStream(QString, int, int)), g_displayGroupManager.get(), SLOT(openPixelStream(QString, int, int)), Qt::QueuedConnection);
    connect(this, SIGNAL(receivedPixelStreamSegement(QString, PixelStreamSegment)), g_displayGroupManager.get(), SLOT(processPixelStreamSegment(QString,PixelStreamSegment)), Qt::QueuedConnection);
    connect(this, SIGNAL(receivedDeletePixelStream(QString)), g_displayGroupManager.get(), SLOT(deletePixelStream(QString)), Qt::QueuedConnection);

    connect(g_displayGroupManager.get(), SIGNAL(pixelStreamViewClosed(QString)), this, SLOT(removePixelStreamer(QString)));

    // get a local DisplayGroupInterface to help manage interaction
    bool success = QMetaObject::invokeMethod(g_displayGroupManager.get(), "getDisplayGroupInterface", Qt::BlockingQueuedConnection, Q_RETURN_ARG(boost::shared_ptr<DisplayGroupInterface>, displayGroupInterface_), Q_ARG(QThread *, QThread::currentThread()));

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

        // if we tried and failed to bind interaction events, try again... maybe the window was created after this new message
        if(!interactionName_.isEmpty() && !interactionBound_)
        {
            put_flog(LOG_DEBUG, "attempting to bind interaction events again...");

            interactionBound_ = bindInteraction();
        }
    }

    // send messages if needed
    if(updatedInteractionState_)
    {
        sendInteractionState();

        updatedInteractionState_ = false;
    }

    // flush the socket
    tcpSocket_->flush();
}

void NetworkListenerThread::socketReceiveMessage()
{
    if(tcpSocket_->state() != QAbstractSocket::ConnectedState)
    {
        emit(finished());
        return;
    }

    // first, read the message header
    QByteArray byteArray = tcpSocket_->read(sizeof(MessageHeader));

    while(byteArray.size() < (int)sizeof(MessageHeader))
    {
        tcpSocket_->waitForReadyRead();

        byteArray.append(tcpSocket_->read(sizeof(MessageHeader) - byteArray.size()));
    }

    // got the header
    MessageHeader * mh = (MessageHeader *)byteArray.data();

    // next, read the actual message
    QByteArray messageByteArray;

    if(mh->size > 0)
    {
        messageByteArray = tcpSocket_->read(mh->size);

        while(messageByteArray.size() < mh->size)
        {
            tcpSocket_->waitForReadyRead();

            messageByteArray.append(tcpSocket_->read(mh->size - messageByteArray.size()));
        }
    }

    // send acknowledgment
    MessageHeader mhAck;
    mhAck.size = 0;
    mhAck.type = MESSAGE_TYPE_ACK;

    int sent = tcpSocket_->write((const char *)&mhAck, sizeof(MessageHeader));

    while(sent < (int)sizeof(MessageHeader))
    {
        sent += tcpSocket_->write((const char *)&mhAck + sent, sizeof(MessageHeader) - sent);
    }

    // we want the ack to be sent immediately
    tcpSocket_->flush();

    while(tcpSocket_->bytesToWrite() > 0)
    {
        tcpSocket_->waitForBytesWritten();
    }

    // got the message
    handleMessage(*mh, messageByteArray);
}

void NetworkListenerThread::setInteractionState(InteractionState interactionState)
{
    updatedInteractionState_ = true;
    interactionState_ = interactionState;
}

void NetworkListenerThread::handleMessage(MessageHeader messageHeader, QByteArray byteArray)
{
    QString uri(messageHeader.uri);

    if(messageHeader.type == MESSAGE_TYPE_QUIT)
    {
        QByteArray empty;
        g_SVGStreamSourceFactory.getObject(uri)->setImageData(empty);

        if (pixelStreamUri_ == uri)
        {
            emit(receivedDeletePixelStream(uri));
            pixelStreamUri_ = QString();
        }
        emit(updatedSVGStreamSource());
    }
    else if(messageHeader.type == MESSAGE_TYPE_PIXELSTREAM)
    {
        // update pixel stream source
        // keep this in this thread so we can have pixel stream source updating and sendPixelStreams() happening in parallel
        // sendPixelStreams() runs in a polling loop on the main thread
        PixelStreamSegment segment;

        // read parameters
        PixelStreamSegmentParameters * parameters = (PixelStreamSegmentParameters *)(byteArray.data());
        segment.parameters = *parameters;

        // just use a unique index for this stream in case the sender does not
        // care or know about the index
        if( segment.parameters.sourceIndex == -1 )
            segment.parameters.sourceIndex = socketDescriptor_;

        // read image data
        QByteArray imageData = byteArray.right(byteArray.size() - sizeof(PixelStreamSegmentParameters));
        segment.imageData = imageData;

        if (pixelStreamUri_.isEmpty())
        {
            emit(receivedOpenPixelStream(uri, segment.parameters.totalWidth, segment.parameters.totalHeight));

            pixelStreamUri_ = uri;
        }

        if (pixelStreamUri_ == uri)
        {
            emit(receivedPixelStreamSegement(uri, segment));
        }
        else
        {
            put_flog(LOG_INFO, "received PixelStreamSegement from incorrect uri: %s", uri.toLocal8Bit().constData());
        }
        // no need to emit any signals since there's a polling loop in the main thread
    }
    else if(messageHeader.type == MESSAGE_TYPE_SVG_STREAM)
    {
        // update SVG stream source
        // similar to pixel streaming above

        g_SVGStreamSourceFactory.getObject(uri)->setImageData(byteArray);

        emit(updatedSVGStreamSource());
    }
    else if(messageHeader.type == MESSAGE_TYPE_BIND_INTERACTION ||
            messageHeader.type == MESSAGE_TYPE_BIND_INTERACTION_EX )
    {
        put_flog(LOG_INFO, "binding to %s", uri.toLocal8Bit().constData());

        interactionName_ = uri;
        interactionExclusive_ = messageHeader.type == MESSAGE_TYPE_BIND_INTERACTION_EX;
        interactionBound_ = bindInteraction();
    }
}

void NetworkListenerThread::removePixelStreamer(QString uri)
{
    if (uri == pixelStreamUri_)
    {
        emit(finished());
    }
}

bool NetworkListenerThread::bindInteraction()
{
    // try to bind to the ContentWindowManager corresponding to interactionName
    boost::shared_ptr<ContentWindowManager> cwm = displayGroupInterface_->getContentWindowManager(interactionName_);

    if(cwm != NULL)
    {
        put_flog(LOG_DEBUG, "found window");

        QMutexLocker locker( cwm->getInteractionBindMutex( ));

        // if an interaction is already bound, don't bind this one if exclusive
        // was requested
        if( cwm->isInteractionBound() && interactionExclusive_ )
        {
            sendBindReply( false );
            return false;
        }

        // todo: disconnect any existing signal connections to the setInteractionState() slot
        // in case we're binding to another window in the same connection / socket

        // make connection to get interaction updates
        cwm->bindInteraction( this, SLOT(setInteractionState(InteractionState)));
        sendBindReply( true );
        return true;
    }
    else
    {
        put_flog(LOG_DEBUG, "could not find window");

        return false;
    }
}

void NetworkListenerThread::sendBindReply( bool successful )
{
    // send message header
    MessageHeader mh;
    mh.size = sizeof(bool);
    mh.type = MESSAGE_TYPE_BIND_INTERACTION_REPLY;

    int sent = tcpSocket_->write((const char *)&mh, sizeof(MessageHeader));

    while(sent < (int)sizeof(MessageHeader))
    {
        sent += tcpSocket_->write((const char *)&mh + sent, sizeof(MessageHeader) - sent);
    }

    tcpSocket_->write((const char *)&successful, sizeof(bool));

    // we want the message to be sent immediately
    tcpSocket_->flush();

    while(tcpSocket_->bytesToWrite() > 0)
    {
        tcpSocket_->waitForBytesWritten();
    }

    interactionName_.clear();
}

void NetworkListenerThread::sendInteractionState()
{
    // send message header
    MessageHeader mh;
    mh.size = sizeof(InteractionState);
    mh.type = MESSAGE_TYPE_INTERACTION;

    int sent = tcpSocket_->write((const char *)&mh, sizeof(MessageHeader));

    while(sent < (int)sizeof(MessageHeader))
    {
        sent += tcpSocket_->write((const char *)&mh + sent, sizeof(MessageHeader) - sent);
    }

    // send interaction state
    sent = tcpSocket_->write((const char *)&interactionState_, sizeof(InteractionState));

    while(sent < (int)sizeof(InteractionState))
    {
        sent += tcpSocket_->write((const char *)&interactionState_ + sent, sizeof(InteractionState) - sent);
    }

    // we want the message to be sent immediately
    tcpSocket_->flush();

    while(tcpSocket_->bytesToWrite() > 0)
    {
        tcpSocket_->waitForBytesWritten();
    }
}
