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

class DisplayGroup : public boost::enable_shared_from_this<DisplayGroup> {

    public:

        boost::shared_ptr<DisplayGroupGraphicsView> getGraphicsView();

        Marker & getMarker();

        void addContent(boost::shared_ptr<Content> content);
        void removeContent(boost::shared_ptr<Content> content);

        std::vector<boost::shared_ptr<Content> > getContents();

        void moveContentToFront(boost::shared_ptr<Content> content);

        void synchronize();

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & marker_;
            ar & contents_;
        }

        QTime timer_;

        // marker
        Marker marker_;

        // vector of all of its contents
        std::vector<boost::shared_ptr<Content> > contents_;

        // used for GUI display
        boost::shared_ptr<DisplayGroupGraphicsView> graphicsView_;
};

#endif
