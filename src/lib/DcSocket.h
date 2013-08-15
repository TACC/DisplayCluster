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

#ifndef DC_SOCKET_H
#define DC_SOCKET_H

#include "../MessageHeader.h"
#include "../InteractionState.h"
#include <QtCore>
#include <queue>

class QTcpSocket;

// we can't use the signal / slot model for handling threads without a Qt event
// loop. so, we make our own thread class and override run()...

class DcSocket : public QThread {

    public:

        DcSocket(const char * hostname, bool async = true );
        ~DcSocket();

        bool isConnected();

        // queue a message to be sent (non-blocking)
        bool queueMessage(QByteArray message);

        // wait for count acks to be received
        void waitForAck(int count=1);

        // -1 for no reply yet, 0 for not bound (if exclusive mode),
        // 1 for successful bound
        int hasInteraction();

        InteractionState getInteractionState();

        int socketDescriptor() const;

        // for synchronous read operations (non-blocking)
        bool hasNewInteractionState();

    protected:

        bool async_;
        QTcpSocket * socket_;

        // mutex and queue for messages to send
        QMutex sendMessagesQueueMutex_;
        std::queue<QByteArray> sendMessagesQueue_;

        // semaphore for ack count
        QSemaphore ackSemaphore_;

        // mutex and flag to trigger socket thread to disconnect
        QMutex disconnectFlagMutex_;
        bool disconnectFlag_;

        // current interaction state
        QMutex interactionStateMutex_;
        InteractionState interactionState_;

        QAtomicInt interactionReply_;

        // socket connections
        bool connect(const char * hostname);
        void disconnect();

        // thread execution
        void run();

        // these are only called in the thread execution
        bool socketSendMessage(QByteArray message);
        bool socketReceiveMessage(MessageHeader & messageHeader, QByteArray & message);

        bool sendMessage_();
        bool receiveMessage_( MESSAGE_TYPE& type );
};

#endif
