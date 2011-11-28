#ifndef CONTENT_WINDOW_H
#define CONTENT_WINDOW_H

#include "ContentWindowInterface.h"
#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/weak_ptr.hpp>

#include "Content.h"
class DisplayGroupManager;

class ContentWindow : public ContentWindowInterface, public boost::enable_shared_from_this<ContentWindow> {

    public:

        ContentWindow() { } // no-argument constructor required for serialization
        ContentWindow(boost::shared_ptr<Content> content);

        boost::shared_ptr<Content> getContent();

        boost::shared_ptr<DisplayGroupManager> getDisplayGroupManager();
        void setDisplayGroupManager(boost::shared_ptr<DisplayGroupManager> displayGroupManager);

        // re-implemented ContentWindowInterface slots
        void moveToFront(ContentWindowInterface * source=NULL);
        void close(ContentWindowInterface * source=NULL);

        // GLWindow rendering
        void render();

        // button dimensions
        void getButtonDimensions(float &width, float &height);

        void dump()
        {
          std::cerr << "x: " << x_ << " " << "y: " << y_ << " "
                    << "w: " << w_ << " " << "w: " << h_ << ": "
                    << content_->getURI() << "\n";
        }

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

typedef boost::shared_ptr<ContentWindow> pContentWindow;

class pyContentWindow
{
public:
  pyContentWindow(pyContent content) { ptr = boost::shared_ptr<ContentWindow>(new ContentWindow(content.get())); }
  ~pyContentWindow() {}

  boost::shared_ptr<ContentWindow> get() const {return ptr;}

private:
  boost::shared_ptr<ContentWindow> ptr;

};

#endif
