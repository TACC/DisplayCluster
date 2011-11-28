#ifndef DISPLAY_GROUP_GRAPHICS_VIEW_PROXY_H
#define DISPLAY_GROUP_GRAPHICS_VIEW_PROXY_H

#include "DisplayGroupInterface.h"
#include <QtGui>

class DisplayGroupGraphicsView;

class DisplayGroupGraphicsViewProxy : public DisplayGroupInterface {

    public:

        DisplayGroupGraphicsViewProxy(boost::shared_ptr<DisplayGroupManager> displayGroupManager);
        ~DisplayGroupGraphicsViewProxy();

        DisplayGroupGraphicsView * getGraphicsView();

        // re-implemented DisplayGroupInterface slots
        void addContentWindow(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source=NULL);
        void removeContentWindow(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source=NULL);
        void moveContentWindowToFront(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source=NULL);

    private:

        // we make this a member since we can't have multiple inheritance of QObject and still use signals/slots
        // see the "Diamond problem"
        DisplayGroupGraphicsView * graphicsView_;
};

#endif
