#ifndef DISPLAY_GROUP_H
#define DISPLAY_GROUP_H

#include <QtGui>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

class Content;
class DisplayGroupGraphicsView;

class DisplayGroup : public boost::enable_shared_from_this<DisplayGroup> {

    public:

        boost::shared_ptr<DisplayGroupGraphicsView> getGraphicsView();

        void addContent(boost::shared_ptr<Content> content);
        std::vector<boost::shared_ptr<Content> > getContents();

        void moveContentToFront(boost::shared_ptr<Content> content);

        void synchronizeContents();

    private:

        // vector of all of its contents
        std::vector<boost::shared_ptr<Content> > contents_;

        // used for GUI display
        boost::shared_ptr<DisplayGroupGraphicsView> graphicsView_;
};

#endif
