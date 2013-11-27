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

#include "StreamPrivate.h"

#include "Stream.h" // For defaultCompressionQuality

#include "Socket.h"
#include "PixelStreamSegment.h"
#include "PixelStreamSegmentParameters.h"

#define SEGMENT_SIZE 512

namespace dc
{

StreamPrivate::StreamPrivate(const std::string &name)
    : name_(name)
    , dcSocket_(0)
    , registeredForEvents_(false)
{
    imageSegmenter_.setNominalSegmentDimensions(SEGMENT_SIZE, SEGMENT_SIZE);
}

bool StreamPrivate::open(const std::string& address)
{
    // Connect to DisplayCluster application
    dcSocket_ = new Socket(address);

    if(!dcSocket_->isConnected())
    {
        delete dcSocket_;
        dcSocket_ = 0;

        return false;
    }

    // Open a window for the PixelStream
    MessageHeader mh(MESSAGE_TYPE_PIXELSTREAM_OPEN, 0, name_);
    return dcSocket_->send(mh, QByteArray());
}

bool StreamPrivate::close()
{
    if( !dcSocket_ || !dcSocket_->isConnected( ))
        return true;

    MessageHeader mh(MESSAGE_TYPE_QUIT, 0, name_);
    dcSocket_->send(mh, QByteArray());

    delete dcSocket_;
    dcSocket_ = 0;

    registeredForEvents_ = false;

    return true;
}

bool StreamPrivate::sendPixelStreamSegment(const PixelStreamSegment &segment)
{
    // Create message header
    size_t segmentSize = sizeof(PixelStreamSegmentParameters) + segment.imageData.size();
    MessageHeader mh(MESSAGE_TYPE_PIXELSTREAM, segmentSize, name_);

    // This byte array will hold the message to be sent over the socket
    QByteArray message;

    // Message payload part 1: segment parameters
    message.append((const char *)(&segment.parameters), sizeof(PixelStreamSegmentParameters));

    // Message payload part 2: image data
    message.append(segment.imageData);

    return dcSocket_->send(mh, message);
}

}
