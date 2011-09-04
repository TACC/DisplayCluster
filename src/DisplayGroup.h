#ifndef DISPLAY_GROUP_H
#define DISPLAY_GROUP_H

#include "Cursor.h"
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

        Cursor & getCursor();

        void addContent(boost::shared_ptr<Content> content);
        std::vector<boost::shared_ptr<Content> > getContents();

        void moveContentToFront(boost::shared_ptr<Content> content);

        void synchronize();

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & cursor_;
            ar & contents_;
        }

        QTime timer_;

        // cursor
        Cursor cursor_;

        // vector of all of its contents
        std::vector<boost::shared_ptr<Content> > contents_;

        // used for GUI display
        boost::shared_ptr<DisplayGroupGraphicsView> graphicsView_;
};

#endif
