#ifndef CONTENT_WINDOW_MANAGER_H
#define CONTENT_WINDOW_MANAGER_H

#include "ContentWindowInterface.h"
#include "Content.h" // need pyContent for pyContentWindowManager
#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/weak_ptr.hpp>

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
        void serialize(Archive & ar, const unsigned int)
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

// typedef needed for SIP
typedef boost::shared_ptr<ContentWindowManager> pContentWindowManager;

class pyContentWindowManager
{
    public:

        pyContentWindowManager(pyContent content)
        {
            boost::shared_ptr<ContentWindowManager> cwm(new ContentWindowManager(content.get()));
            ptr_ = cwm;
        }

        pyContentWindowManager(boost::shared_ptr<ContentWindowManager> cwm)
        {
            ptr_ = cwm;
        }

        boost::shared_ptr<ContentWindowManager> get()
        {
            return ptr_;
        }

        pyContent getPyContent()
        {
            return pyContent(get()->getContent());
        }

    private:

        boost::shared_ptr<ContentWindowManager> ptr_;
};

#endif
