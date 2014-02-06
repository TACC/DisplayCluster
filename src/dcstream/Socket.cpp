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

#include "Socket.h"

#include "MessageHeader.h"
#include "NetworkProtocol.h"
#include "log.h"

#include <QtNetwork/QTcpSocket>
#include <QThread>

#define RECEIVE_TIMEOUT_MS                 1000
#define WAIT_FOR_BYTES_WRITTEN_TIMEOUT_MS  1000

namespace dc
{

const unsigned short Socket::defaultPortNumber_ = 1701;

Socket::Socket(const std::string &hostname, const unsigned short port)
    : socket_(new QTcpSocket())
{
    if( !connect( hostname, port ))
    {
        put_flog(LOG_ERROR, "could not connect to host %s:%i", hostname.c_str(), port);
    }
    // Forward disconnection events
    QObject::connect(socket_, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
}

Socket::~Socket()
{
    socket_->flush();
    delete socket_;
}

bool Socket::isConnected() const
{
    return socket_->state() == QTcpSocket::ConnectedState;
}

int Socket::getFileDescriptor() const
{
    return socket_->socketDescriptor();
}

bool Socket::hasMessage(const size_t messageSize) const
{
    return socket_->bytesAvailable() >= (int)(MessageHeader::serializedSize + messageSize);
}

bool Socket::send(const MessageHeader& messageHeader, const QByteArray &message)
{
    // Send header
    if ( !send(messageHeader) )
        return false;

    if (message.isEmpty())
        return true;

    // Send message data
    const char* data = message.constData();
    const int size = message.size();

    int sent = socket_->write(data, size);

    while(sent < size && isConnected())
    {
        sent += socket_->write(data + sent, size - sent);
    }

    // Needed in the absence of event loop, otherwise the reception is frozen as well...
    while(socket_->bytesToWrite() > 0 && isConnected())
    {
        socket_->waitForBytesWritten();
    }

    return sent == size;
}

bool Socket::send(const MessageHeader& messageHeader)
{
    QDataStream stream(socket_);
    stream << messageHeader;

    return stream.status() == QDataStream::Ok;
}

bool Socket::receive(MessageHeader & messageHeader, QByteArray & message)
{
    if (!receive(messageHeader))
        return false;

    // get the message
    if(messageHeader.size > 0)
    {
        message = socket_->read(messageHeader.size);

        while(message.size() < int(messageHeader.size))
        {
            if ( !socket_->waitForReadyRead(RECEIVE_TIMEOUT_MS) )
                return false;

            message.append(socket_->read(messageHeader.size - message.size()));
        }
    }

    if (messageHeader.type == MESSAGE_TYPE_QUIT)
    {
        put_flog(LOG_DEBUG, "Received QUIT - disconnecting");
        socket_->disconnectFromHost();
        return false;
    }

    return true;
}

bool Socket::receive(MessageHeader & messageHeader)
{
    while( socket_->bytesAvailable() < qint64(MessageHeader::serializedSize) )
    {
        if ( !socket_->waitForReadyRead(RECEIVE_TIMEOUT_MS) )
            return false;
    }

    QDataStream stream(socket_);
    stream >> messageHeader;

    return stream.status() == QDataStream::Ok;
}

bool Socket::connect(const std::string& hostname, const unsigned short port)
{
    // make sure we're disconnected
    socket_->disconnectFromHost();

    // open connection
    socket_->connectToHost(hostname.c_str(), port);

    if(!socket_->waitForConnected(RECEIVE_TIMEOUT_MS))
    {
        put_flog(LOG_ERROR, "could not connect to host %s:%i", hostname.c_str(), port);
        return false;
    }

    // handshake
    if( checkProtocolVersion( ))
    {
        put_flog(LOG_INFO, "connected to host %s", hostname.c_str());
        return true;
    }

    put_flog( LOG_ERROR, "Protocol version check failed for host: %s:%i",
              hostname.c_str(), port );
    socket_->disconnectFromHost();
    return false;
}

bool Socket::checkProtocolVersion()
{
    while( socket_->bytesAvailable() < qint64(sizeof(int32_t)) )
    {
        if( !socket_->waitForReadyRead( RECEIVE_TIMEOUT_MS ))
            return false;
    }

    int32_t protocolVersion = -1;
    socket_->read((char *)&protocolVersion, sizeof(int32_t));

    if( protocolVersion == NETWORK_PROTOCOL_VERSION )
        return true;

    put_flog( LOG_ERROR, "unsupported protocol version %i != %i",
              protocolVersion, NETWORK_PROTOCOL_VERSION );
    return false;
}

}
