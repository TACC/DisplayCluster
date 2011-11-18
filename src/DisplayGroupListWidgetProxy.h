#ifndef DISPLAY_GROUP_LIST_WIDGET_PROXY_H
#define DISPLAY_GROUP_LIST_WIDGET_PROXY_H

#include "DisplayGroupInterface.h"
#include <QtGui>

class DisplayGroupListWidgetProxy : public DisplayGroupInterface {

    public:

        DisplayGroupListWidgetProxy(boost::shared_ptr<DisplayGroup> displayGroup);
        ~DisplayGroupListWidgetProxy();

        QListWidget * getListWidget();

        // re-implemented DisplayGroupInterface slots
        void addContentWindow(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source=NULL);
        void removeContentWindow(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source=NULL);
        void moveContentWindowToFront(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source=NULL);

    private:

        // we make this a member since we can't have multiple inheritance of QObject and still use signals/slots
        // see the "Diamond problem"
        QListWidget * listWidget_;

        void refreshListWidget();
};

#endif
