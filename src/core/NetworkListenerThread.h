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

#ifndef NETWORK_LISTENER_THREAD_H
#define NETWORK_LISTENER_THREAD_H

#include "MessageHeader.h"
#include "Event.h"
#include "PixelStreamSegment.h"
#include "EventReceiver.h"

#include <QtNetwork/QTcpSocket>
#include <QQueue>

using dc::Event;
using dc::PixelStreamSegment;

class NetworkListenerThread : public EventReceiver
{
    Q_OBJECT

public:

    NetworkListenerThread(int socketDescriptor);
    ~NetworkListenerThread();

public slots:

    void processEvent(Event event);
    void pixelStreamerClosed(QString uri);

    void eventRegistrationReply(QString uri, bool success);

signals:

    void finished();

    void receivedAddPixelStreamSource(QString uri, size_t sourceIndex);
    void receivedPixelStreamSegement(QString uri, size_t SourceIndex, PixelStreamSegment segment);
    void receivedPixelStreamFinishFrame(QString uri, size_t SourceIndex);
    void receivedRemovePixelStreamSource(QString uri, size_t sourceIndex);

    void registerToEvents(QString uri, bool exclusive, EventReceiver* receiver);

    void receivedCommand(QString command, QString senderUri);

    /** @internal */
    void dataAvailable();

private slots:

    void initialize();
    void process();
    void socketReceiveMessage();

private:

    int socketDescriptor_;
    QTcpSocket* tcpSocket_;

    QString pixelStreamUri_;

    bool registeredToEvents_;
    QQueue<Event> events_;

    MessageHeader receiveMessageHeader();
    QByteArray receiveMessageBody(const int size);

    void handleMessage(const MessageHeader& messageHeader, const QByteArray& byteArray);
    void handlePixelStreamMessage(const QString& uri, const QByteArray& byteArray);

    void sendProtocolVersion();
    void sendBindReply(const bool successful);
    void send(const Event &event);
    void sendQuit();
    bool send(const MessageHeader& messageHeader);
};

#endif
