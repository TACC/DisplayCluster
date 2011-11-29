#ifndef DISPLAY_GROUP_MANAGER_H
#define DISPLAY_GROUP_MANAGER_H

#include "DisplayGroupInterface.h"
#include "Marker.h"
#include <QtGui>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

class ContentWindowManager;

enum MESSAGE_TYPE { MESSAGE_TYPE_CONTENTS, MESSAGE_TYPE_CONTENTS_DIMENSIONS, MESSAGE_TYPE_PIXELSTREAM, MESSAGE_TYPE_FRAME_CLOCK, MESSAGE_TYPE_QUIT };

#define MESSAGE_HEADER_URI_LENGTH 64

struct MessageHeader {
    int size;
    MESSAGE_TYPE type;
    char uri[MESSAGE_HEADER_URI_LENGTH]; // optional URI related to message. needs to be a fixed size so sizeof(MessageHeader) is constant
};

class DisplayGroupManager : public DisplayGroupInterface, public boost::enable_shared_from_this<DisplayGroupManager> {
    Q_OBJECT

    public:

        DisplayGroupManager();

        Marker & getMarker();
        boost::shared_ptr<boost::posix_time::ptime> getTimestamp();

        // re-implemented DisplayGroupInterface slots
        void addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);

    public slots:

        void handleMessage(MessageHeader messageHeader, QByteArray byteArray);

        void receiveMessages();

        void sendDisplayGroup();
        void sendContentsDimensionsRequest();
        void sendPixelStreams();
        void sendFrameClockUpdate();
        void receiveFrameClockUpdate();
        void sendQuit();

        void advanceContents();

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & marker_;
            ar & contentWindowManagers_;
        }

        // marker
        Marker marker_;

        // frame timing
        boost::shared_ptr<boost::posix_time::ptime> timestamp_;

        void receiveDisplayGroup(MessageHeader messageHeader);
        void receiveContentsDimensionsRequest(MessageHeader messageHeader);
        void receivePixelStreams(MessageHeader messageHeader);
};

#endif
