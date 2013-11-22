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

#include "Stream.h"

#include "log.h"

#include "StreamPrivate.h"
#include "Socket.h"
#include "ImageWrapper.h"
#include "PixelStreamSegment.h"
#include "PixelStreamSegmentParameters.h"

#include <stdexcept>

namespace dc
{

Stream::Stream(const std::string& name, const std::string& address)
    : impl_(new StreamPrivate(name))
{
    if(name.empty())
    {
        put_flog(LOG_ERROR, "Invalid Stream name");
    }

    if (!impl_->open(address))
    {
        put_flog(LOG_ERROR, "Stream could not connect to host: %s", address.c_str());
    }
}

Stream::~Stream()
{
    impl_->close();
    delete impl_;
}

bool Stream::isConnected() const
{
    return impl_->dcSocket_ && impl_->dcSocket_->isConnected();
}

bool Stream::send(const ImageWrapper& image)
{
//    if (!(image.compressionPolicy == COMPRESSION_ON) && image.pixelFormat != dc::ARGB)
//    {
//        put_flog(LOG_ERROR, "Currently, RAW images only be sent in ARGB format. Other formats remain to be implemented.");
//        return false;
//    }

    PixelStreamSegments segments = impl_->imageSegmenter_.generateSegments(image);

    bool allSuccess = true;
    for(PixelStreamSegments::const_iterator it = segments.begin(); it!=segments.end(); it++)
    {
        allSuccess = allSuccess && impl_->sendPixelStreamSegment(*it);
    }
    return allSuccess;
}

bool Stream::finishFrame()
{
    // Open a window for the PixelStream
    MessageHeader mh = impl_->createMessageHeader(MESSAGE_TYPE_PIXELSTREAM_FINISH_FRAME, 0);
    return impl_->dcSocket_->send(mh, QByteArray());
}

bool Stream::bindInteraction(const bool exclusive)
{
    if(!isConnected())
    {
        put_flog(LOG_WARN, "dcSocket is NULL or not connected");
        return false;
    }

    MESSAGE_TYPE type = exclusive ? MESSAGE_TYPE_BIND_INTERACTION_EX :
                                    MESSAGE_TYPE_BIND_INTERACTION;
    MessageHeader mh = impl_->createMessageHeader(type, 0);

    // Send the bind message
    if( !impl_->dcSocket_->send(mh, QByteArray()) )
    {
        put_flog(LOG_ERROR, "Could not send bind message");
        return false;
    }

    // Wait for bind reply
    QByteArray message;
    bool success = impl_->dcSocket_->receive(mh, message);
    if(!success || mh.type != MESSAGE_TYPE_BIND_INTERACTION_REPLY)
    {
        put_flog(LOG_ERROR, "Invalid reply from host");
        return false;
    }

    impl_->interactionBound_= *(bool*)(message.data());

    return isInteractionBound();
}

bool Stream::isInteractionBound() const
{
    return impl_->interactionBound_;
}

int Stream::getDescriptor() const
{
    return impl_->dcSocket_->getFileDescriptor();
}

bool Stream::hasInteractionState() const
{
    return impl_->dcSocket_->hasMessage(sizeof(InteractionState));
}

InteractionState Stream::retrieveInteractionState()
{
    MessageHeader mh;
    QByteArray message;

    bool success = impl_->dcSocket_->receive(mh, message);

    if(!success || mh.type != MESSAGE_TYPE_INTERACTION)
    {
        put_flog(LOG_ERROR, "Invalid reply from host");
        return InteractionState();
    }

    return *(InteractionState *)(message.data());
}

}
