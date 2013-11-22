#include "DcStream.h"

#include "log.h"

#include "DcStreamPrivate.h"
#include "DcSocket.h"
#include "DcImageWrapper.h"
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
