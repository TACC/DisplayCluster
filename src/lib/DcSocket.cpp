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

#include "DcSocket.h"
#include "../NetworkProtocol.h"
#include "../log.h"
#include <QtNetwork/QTcpSocket>

DcSocket::DcSocket(const char * hostname, bool async)
    : interactionReply_( -1 )
{
    // defaults
    socket_ = NULL;
    disconnectFlag_ = false;
    async_ = async;

    if(connect(hostname) != true)
    {
        put_flog(LOG_ERROR, "could not connect to host %s", hostname);
    }
}

DcSocket::~DcSocket()
{
    disconnect();
}

bool DcSocket::isConnected()
{
    // if the thread is running, we are connected
    // the thread will exit if there is a connection error
    return async_ ? isRunning() : (socket_ ? socket_->state() == QTcpSocket::ConnectedState : false);
}

bool DcSocket::queueMessage(QByteArray message)
{
    // only queue the message if we're connected
    if(isConnected() != true)
    {
        return false;
    }

    {
        QMutexLocker locker(&sendMessagesQueueMutex_);
        sendMessagesQueue_.push(message);
    }

    if( !async_ )
        sendMessage_();

    return true;
}

void DcSocket::waitForAck(int count)
{
    // only wait if we're connected
    if(isConnected() != true)
    {
        put_flog(LOG_WARN, "not connected");

        return;
    }

    if( async_ )
        ackSemaphore_.acquire(count);
}

int DcSocket::hasInteraction()
{
    MESSAGE_TYPE type;
    if( !receiveMessage_( type ))
        return -1;
    return interactionReply_;
}

InteractionState DcSocket::getInteractionState()
{
    QMutexLocker locker(&interactionStateMutex_);

    return interactionState_;
}

int DcSocket::socketDescriptor() const
{
    return socket_->socketDescriptor();
}

bool DcSocket::hasNewInteractionState()
{
    MESSAGE_TYPE type;
    if( !receiveMessage_( type ))
        return false;
    return type == MESSAGE_TYPE_INTERACTION;
}

bool DcSocket::connect(const char * hostname)
{
    // make sure we're disconnected
    disconnect();

    // reset everything
    sendMessagesQueue_ = std::queue<QByteArray>();
    ackSemaphore_.acquire(ackSemaphore_.available()); // should reset semaphore to 0
    disconnectFlag_ = false;

    socket_ = new QTcpSocket();

    // open connection
    socket_->connectToHost(hostname, 1701);

    if(socket_->waitForConnected() != true)
    {
        put_flog(LOG_ERROR, "could not connect to host %s", hostname);

        delete socket_;
        socket_ = NULL;

        return false;
    }

    // todo: we need to consider the performance of the low delay option
    // socket_->setSocketOption(QAbstractSocket::LowDelayOption, 1);

    // handshake
    while(socket_->bytesAvailable() < (int)sizeof(int32_t))
    {
        socket_->waitForReadyRead();

    #ifndef _WIN32
        usleep(10);
    #endif
    }

    int32_t protocolVersion = -1;
    socket_->read((char *)&protocolVersion, sizeof(int32_t));

    if(protocolVersion != NETWORK_PROTOCOL_VERSION)
    {
        socket_->disconnectFromHost();

        put_flog(LOG_ERROR, "unsupported protocol version %i != %i", protocolVersion, NETWORK_PROTOCOL_VERSION);

        delete socket_;
        socket_ = NULL;

        return false;
    }

    // move the socket to this thread (which is about to start)
    socket_->moveToThread(this);

    // start thread execution
    if( async_ )
        start();

    put_flog(LOG_INFO, "connected to host %s", hostname);

    return true;
}

void DcSocket::disconnect()
{
    if(isConnected() == true)
    {
        {
            QMutexLocker locker(&disconnectFlagMutex_);
            disconnectFlag_ = true;
        }

        if( async_ )
        {
            // wait for thread to finish
            bool success = wait();

            if(success == false)
            {
                put_flog(LOG_ERROR, "thread did not finish");
            }
        }
        else
        {
            // delete the socket
            delete socket_;
            socket_ = NULL;

            put_flog(LOG_DEBUG, "finished");
        }
    }
}

void DcSocket::run()
{
    put_flog(LOG_DEBUG, "started");

    while(true)
    {
        // break here if we had a failure
        if( !sendMessage_( ))
            break;

        MESSAGE_TYPE type;
        receiveMessage_( type );

        // make sure the socket is still connected
        if(socket_->state() != QAbstractSocket::ConnectedState)
        {
            put_flog(LOG_ERROR, "socket disconnected");
            break;
        }

        // flush the socket
        socket_->flush();

        // break if disconnect() was called
        {
            QMutexLocker locker(&disconnectFlagMutex_);

            if(disconnectFlag_ == true)
            {
                break;
            }
        }
    }

    // delete the socket
    delete socket_;
    socket_ = NULL;

    put_flog(LOG_DEBUG, "finished");
}

bool DcSocket::sendMessage_()
{
    // send a message if available
    QByteArray sendMessage;

    {
        QMutexLocker locker(&sendMessagesQueueMutex_);

        if(sendMessagesQueue_.size() > 0)
        {
            sendMessage = sendMessagesQueue_.front();
            sendMessagesQueue_.pop();
        }
    }

    if(sendMessage.isEmpty() != true)
    {
        bool success = socketSendMessage(sendMessage);

        if(success != true)
        {
            put_flog(LOG_ERROR, "error sending message");
            return false;
        }
    }
    return true;
}

bool DcSocket::receiveMessage_( MESSAGE_TYPE& type )
{
    type = MESSAGE_TYPE_NONE;

    // receive a message if available
    socket_->waitForReadyRead(1);

    if(socket_->bytesAvailable() < (int)sizeof(MessageHeader))
        return false;

    MessageHeader messageHeader;
    QByteArray message;

    bool success = socketReceiveMessage(messageHeader, message);

    if( !success )
    {
        put_flog(LOG_ERROR, "error receiving message");
        return false;
    }

    type = messageHeader.type;

    // handle the message
    if(type == MESSAGE_TYPE_ACK)
    {
        ackSemaphore_.release(1);
    }
    else if(type == MESSAGE_TYPE_INTERACTION)
    {
        QMutexLocker locker(&interactionStateMutex_);
        interactionState_ = *(InteractionState *)(message.data());
    }
    else if(type == MESSAGE_TYPE_BIND_INTERACTION_REPLY)
    {
        interactionReply_.fetchAndStoreAcquire( *(bool*)(message.data()) ? 1 : 0 );
    }
    else
    {
        put_flog(LOG_ERROR, "unknown message header type %i", type);
        return false;
    }
    return true;
}

bool DcSocket::socketSendMessage(QByteArray message)
{
    if(socket_->state() != QAbstractSocket::ConnectedState)
    {
        return false;
    }

    char * data = message.data();
    int size = message.size();

    int sent = socket_->write(data, size);

    while(sent < size && socket_->state() == QAbstractSocket::ConnectedState)
    {
        sent += socket_->write(data + sent, size - sent);
    }

    if(sent != size)
    {
        return false;
    }

    socket_->flush();

    while(socket_->bytesToWrite() > 0)
    {
        socket_->waitForBytesWritten();
    }

    return true;
}

bool DcSocket::socketReceiveMessage(MessageHeader & messageHeader, QByteArray & message)
{
    if(socket_->state() != QAbstractSocket::ConnectedState)
    {
        return false;
    }

    QByteArray byteArray = socket_->read(sizeof(MessageHeader));

    while(byteArray.size() < (int)sizeof(MessageHeader))
    {
        socket_->waitForReadyRead();

        byteArray.append(socket_->read(sizeof(MessageHeader) - byteArray.size()));
    }

    // got the header
    messageHeader = *(MessageHeader *)byteArray.data();

    // get the message
    if(messageHeader.size > 0)
    {
        message = socket_->read(messageHeader.size);

        while(message.size() < messageHeader.size)
        {
            socket_->waitForReadyRead();

            message.append(socket_->read(messageHeader.size - message.size()));
        }
    }

    return true;
}
