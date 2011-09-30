#ifndef DISPLAY_GROUP_H
#define DISPLAY_GROUP_H

#include "Marker.h"
#include <QtGui>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

class Content;
class DisplayGroupGraphicsView;

enum MESSAGE_TYPE { MESSAGE_TYPE_CONTENTS, MESSAGE_TYPE_PIXELSTREAM };

struct MessageHeader {
    int size;
    MESSAGE_TYPE type;
};

class DisplayGroup : public boost::enable_shared_from_this<DisplayGroup> {

    public:

        boost::shared_ptr<DisplayGroupGraphicsView> getGraphicsView();

        Marker & getMarker();

        void addContent(boost::shared_ptr<Content> content);
        void removeContent(boost::shared_ptr<Content> content);
        void removeContent(std::string uri);
        std::vector<boost::shared_ptr<Content> > getContents();

        void moveContentToFront(boost::shared_ptr<Content> content);

        void synchronize();

        void sendDisplayGroup();
        void sendPixelStreams();

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & marker_;
            ar & contents_;
        }

        // marker
        Marker marker_;

        // vector of all of its contents
        std::vector<boost::shared_ptr<Content> > contents_;

        // used for GUI display
        boost::shared_ptr<DisplayGroupGraphicsView> graphicsView_;

        void receiveDisplayGroup(MessageHeader messageHeader);
        void receivePixelStreams(MessageHeader messageHeader);
};

#endif
