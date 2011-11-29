#ifndef DISPLAY_GROUP_LIST_WIDGET_PROXY_H
#define DISPLAY_GROUP_LIST_WIDGET_PROXY_H

#include "DisplayGroupInterface.h"
#include <QtGui>

class DisplayGroupListWidgetProxy : public DisplayGroupInterface {

    public:

        DisplayGroupListWidgetProxy(boost::shared_ptr<DisplayGroupManager> displayGroupManager);
        ~DisplayGroupListWidgetProxy();

        QListWidget * getListWidget();

        // re-implemented DisplayGroupInterface slots
        void addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);

    private:

        // we make this a member since we can't have multiple inheritance of QObject and still use signals/slots
        // see the "Diamond problem"
        QListWidget * listWidget_;

        void refreshListWidget();
};

#endif
