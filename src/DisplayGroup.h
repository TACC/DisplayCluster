#ifndef DISPLAY_GROUP_H
#define DISPLAY_GROUP_H

#include <QtGui>
#include <vector>
#include <boost/shared_ptr.hpp>

class Content;
class DisplayGroupGraphicsView;

class DisplayGroup {

    public:

        boost::shared_ptr<DisplayGroupGraphicsView> getGraphicsView();

        void addContent(boost::shared_ptr<Content> content);
        std::vector<boost::shared_ptr<Content> > getContents();

        void synchronizeContents();

    private:

        // vector of all of its contents
        std::vector<boost::shared_ptr<Content> > contents_;

        // used for GUI display
        boost::shared_ptr<DisplayGroupGraphicsView> graphicsView_;
};

#endif
