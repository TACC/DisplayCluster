#ifndef CONTENT_WINDOW_MANAGER_H
#define CONTENT_WINDOW_MANAGER_H

#include "ContentWindowInterface.h"
#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/weak_ptr.hpp>

class Content;
class DisplayGroupManager;

class ContentWindowManager : public ContentWindowInterface, public boost::enable_shared_from_this<ContentWindowManager> {

    public:

        ContentWindowManager() { } // no-argument constructor required for serialization
        ContentWindowManager(boost::shared_ptr<Content> content);

        boost::shared_ptr<Content> getContent();

        boost::shared_ptr<DisplayGroupManager> getDisplayGroupManager();
        void setDisplayGroupManager(boost::shared_ptr<DisplayGroupManager> displayGroupManager);

        // re-implemented ContentWindowInterface slots
        void moveToFront(ContentWindowInterface * source=NULL);
        void close(ContentWindowInterface * source=NULL);

        // GLWindow rendering
        void render();

    protected:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & content_;
            ar & displayGroupManager_;
            ar & contentWidth_;
            ar & contentHeight_;
            ar & x_;
            ar & y_;
            ar & w_;
            ar & h_;
            ar & centerX_;
            ar & centerY_;
            ar & zoom_;
            ar & selected_;
        }

    private:

        boost::shared_ptr<Content> content_;

        boost::weak_ptr<DisplayGroupManager> displayGroupManager_;
};

#endif
