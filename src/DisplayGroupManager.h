#ifndef DISPLAY_GROUP_MANAGER_H
#define DISPLAY_GROUP_MANAGER_H

#include "DisplayGroupInterface.h"
#include "Options.h"
#include "Marker.h"
#include "config.h"
#include <QtGui>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#if ENABLE_SKELETON_SUPPORT
#include "SkeletonThread.h"
#endif

class ContentWindowManager;

enum MESSAGE_TYPE { MESSAGE_TYPE_CONTENTS, MESSAGE_TYPE_CONTENTS_DIMENSIONS, MESSAGE_TYPE_PIXELSTREAM, MESSAGE_TYPE_FRAME_CLOCK, MESSAGE_TYPE_QUIT, MESSAGE_TYPE_SKELETONS };

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

#if ENABLE_SKELETON_SUPPORT
        std::vector<SkeletonState> getSkeletons();
#endif

        // re-implemented DisplayGroupInterface slots
        void addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);

    public slots:

        void receiveMessages();

        void sendDisplayGroup();
        void sendContentsDimensionsRequest();
        void sendPixelStreams();
        void sendFrameClockUpdate();
        void receiveFrameClockUpdate();
        void sendQuit();
#if ENABLE_SKELETON_SUPPORT
        void setSkeletons(std::vector<SkeletonState> skeletons);
#endif
        void advanceContents();

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & options_;
            ar & markers_;
            ar & contentWindowManagers_;
#if ENABLE_SKELETON_SUPPORT
            ar & skeletons_;
#endif
        }

        QMutex markersMutex_;

        // options
        boost::shared_ptr<Options> options_;

        // marker
        std::vector<boost::shared_ptr<Marker> > markers_;

        // frame timing
        boost::shared_ptr<boost::posix_time::ptime> timestamp_;

#if ENABLE_SKELETON_SUPPORT
        std::vector<SkeletonState> skeletons_;
#endif

        void receiveDisplayGroup(MessageHeader messageHeader);
        void receiveContentsDimensionsRequest(MessageHeader messageHeader);
        void receivePixelStreams(MessageHeader messageHeader);

#if ENABLE_SKELETON_SUPPORT
        void receiveSkeletons(MessageHeader messageHeader);
#endif
};

#endif
