/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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

#include "DcStreamEventThread.h"

DcStreamEventThread::DcStreamEventThread()
{
}

void DcStreamEventThread::storeNewInteractionState(InteractionState state)
{
    QMutexLocker lock(&interactionStateMutex_);
    interactionStates_.enqueue(state);
}

bool DcStreamEventThread::hasPendingInteractionState() const
{
    QMutexLocker lock(&interactionStateMutex_);
    return !interactionStates_.empty();
}

InteractionState DcStreamEventThread::retrieveInteractionState()
{
    QMutexLocker lock(&interactionStateMutex_);

    if (interactionStates_.empty())
        return InteractionState();

    return interactionStates_.dequeue();
}

void DcStreamEventThread::run()
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

bool DcStreamEventThread::queueMessage(QByteArray message)
{
    // only queue the message if we're connected
    if(!isConnected())
    {
        return false;
    }

    {
        QMutexLocker locker(&sendMessagesQueueMutex_);
        sendMessagesQueue_.push(message);
    }

    sendMessage_();

    return true;
}

bool DcStreamEventThread::sendMessage_()
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

    if(!sendMessage.isEmpty())
    {
        bool success = socketSendMessage(sendMessage);

        if(!success)
        {
            put_flog(LOG_ERROR, "error sending message");
            return false;
        }
    }
    return true;
}

bool DcStreamEventThread::receiveMessage_( MESSAGE_TYPE& type )
{
    type = MESSAGE_TYPE_NONE;

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
    }
    else if(type == MESSAGE_TYPE_INTERACTION)
    {
        InteractionState interactionState = *(InteractionState *)(message.data());
        emit received(interactionState);
    }
    else if(type == MESSAGE_TYPE_BIND_INTERACTION_REPLY)
    {
        bool success = *(bool*)(message.data());
        emit receivedInteractionBindReply(success);
    }
    else if(type == MESSAGE_TYPE_QUIT)
    {
        put_flog(LOG_INFO, "Received QUIT - TODO implement this action");
    }
    else
    {
        put_flog(LOG_ERROR, "unknown message header type %i", type);
        return false;
    }
    return true;
}

