#ifndef DISPLAY_GROUP_MANAGER_H
#define DISPLAY_GROUP_MANAGER_H

#include "DisplayGroupInterface.h"
#include "Options.h"
#include "Marker.h"
#include <QtGui>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "ContentWindowManager.h"

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

        boost::shared_ptr<Options> getOptions();

        boost::shared_ptr<Marker> getNewMarker();
        std::vector<boost::shared_ptr<Marker> > getMarkers();

        boost::shared_ptr<boost::posix_time::ptime> getTimestamp();

        // re-implemented DisplayGroupInterface slots
        void addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);

        // find the offset between the rank 0 clock and the rank 1 clock. recall the rank 1 clock is used across rank 1 - n.
        void calibrateTimestampOffset();

    public slots:

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
            ar & options_;
            ar & markers_;
            ar & contentWindowManagers_;
        }

        // options
        boost::shared_ptr<Options> options_;

        // marker
        std::vector<boost::shared_ptr<Marker> > markers_;

        // frame timing
        boost::shared_ptr<boost::posix_time::ptime> timestamp_;

        // rank 1 - rank 0 timestamp offset
        boost::posix_time::time_duration timestampOffset_;

        void receiveDisplayGroup(MessageHeader messageHeader);
        void receiveContentsDimensionsRequest(MessageHeader messageHeader);
        void receivePixelStreams(MessageHeader messageHeader);
};

#endif
