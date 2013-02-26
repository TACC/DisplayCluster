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
#include "main.h"
#include "log.h"
#include "PixelStreamSource.h"
#include "ParallelPixelStream.h"
#include "SVGStreamSource.h"
#include <stdint.h>

NetworkListenerThread::NetworkListenerThread(int socketDescriptor)
{
    // assign values
    socketDescriptor_ = socketDescriptor;

    // required for using MessageHeader with queued connections
    qRegisterMetaType<MessageHeader>("MessageHeader");

    // connect signals
    connect(this, SIGNAL(updatedPixelStreamSource()), g_displayGroupManager.get(), SLOT(sendPixelStreams()), Qt::BlockingQueuedConnection);

    connect(this, SIGNAL(updatedSVGStreamSource()), g_displayGroupManager.get(), SLOT(sendSVGStreams()), Qt::BlockingQueuedConnection);
}

void NetworkListenerThread::run()
{
    QTcpSocket tcpSocket;

    if(tcpSocket.setSocketDescriptor(socketDescriptor_) != true)
    {
        put_flog(LOG_ERROR, "could not set socket descriptor: %s", tcpSocket.errorString().toStdString().c_str());
        return;
    }

    // handshake
    int32_t protocolVersion = NETWORK_PROTOCOL_VERSION;
    tcpSocket.write((char *)&protocolVersion, sizeof(int32_t));

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
        QByteArray messageByteArray;

        if(mh->size > 0)
        {
            messageByteArray = tcpSocket.read(mh->size);

            while(messageByteArray.size() < mh->size)
            {
                tcpSocket.waitForReadyRead();

                messageByteArray.append(tcpSocket.read(mh->size - messageByteArray.size()));
            }
        }

        // got the message
        handleMessage(*mh, messageByteArray);

        // send acknowledgment
        const char ack[] = "ack";

        int sent = tcpSocket.write(ack, 3);

        while(sent < 3)
        {
            sent += tcpSocket.write(ack + sent, 3 - sent);
        }
    }

    tcpSocket.disconnectFromHost();

    put_flog(LOG_DEBUG, "disconnected");
}

void NetworkListenerThread::handleMessage(MessageHeader messageHeader, QByteArray byteArray)
{
    if(messageHeader.type == MESSAGE_TYPE_PIXELSTREAM)
    {
        // update pixel stream source
        // keep this in this thread so we can have pixel stream source updating and sendPixelStreams() happening in parallel
        // sendPixelStreams() slot executions may still stack up, but they'll each grab only the latest pixel stream data
        std::string uri(messageHeader.uri);

        g_pixelStreamSourceFactory.getObject(uri)->setImageData(byteArray);

        emit(updatedPixelStreamSource());
    }
    else if(messageHeader.type == MESSAGE_TYPE_PIXELSTREAM_DIMENSIONS_CHANGED)
    {
        std::string uri(messageHeader.uri);

        const int * dimensions = (const int *)byteArray.constData();

        g_pixelStreamSourceFactory.getObject(uri)->setDimensions(dimensions[0], dimensions[1]);

        emit(updatedPixelStreamSource());
    }
    else if(messageHeader.type == MESSAGE_TYPE_PARALLEL_PIXELSTREAM)
    {
        // update parallel pixel stream source
        // keep this in this thread so we can have parallel pixel stream source updating and sendParallelPixelStreams() happening in parallel
        // sendParallelPixelStreams() runs in a polling loop on the main thread
        std::string uri(messageHeader.uri);

        ParallelPixelStreamSegment segment;

        // read parameters
        ParallelPixelStreamSegmentParameters * parameters = (ParallelPixelStreamSegmentParameters *)(byteArray.data());
        segment.parameters = *parameters;

        // read image data
        QByteArray imageData = byteArray.right(byteArray.size() - sizeof(ParallelPixelStreamSegmentParameters));
        segment.imageData = imageData;

        g_parallelPixelStreamSourceFactory.getObject(uri)->insertSegment(segment);

        // no need to emit any signals since there's a polling loop in the main thread
    }
    else if(messageHeader.type == MESSAGE_TYPE_SVG_STREAM)
    {
        // update SVG stream source
        // similar to pixel streaming above
        std::string uri(messageHeader.uri);

        g_SVGStreamSourceFactory.getObject(uri)->setImageData(byteArray);

        emit(updatedSVGStreamSource());
    }
}
