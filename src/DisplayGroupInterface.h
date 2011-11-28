#ifndef DISPLAY_GROUP_INTERFACE_H
#define DISPLAY_GROUP_INTERFACE_H

#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "ContentWindow.h"
class DisplayGroupManager;

class DisplayGroupInterface : public QObject {
    Q_OBJECT

    public:

        DisplayGroupInterface() { }
        DisplayGroupInterface(boost::shared_ptr<DisplayGroupManager> displayGroupManager);

        boost::shared_ptr<DisplayGroupManager> getDisplayGroupManager();

        std::vector<boost::shared_ptr<ContentWindow> > getContentWindows();
        bool hasContent(std::string uri);

        // remove all current ContentWindows and add the vector of provided ContentWindows
        void setContentWindows(std::vector<boost::shared_ptr<ContentWindow> > contentWindows);

 	void dump();

    public slots:

        // these methods set the local copies of the state variables if source != this
        // they will emit signals if source == NULL or if this is a DisplayGroup object
        // the source argument should not be provided by users -- only by these functions
        virtual void addContentWindow(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source=NULL);
        virtual void removeContentWindow(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source=NULL);
        virtual void moveContentWindowToFront(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source=NULL);

    signals:

        // emitting these signals will trigger updates on the corresponding DisplayGroup
        // as well as all other DisplayGroupInterfaces to that DisplayGroup
        void contentWindowAdded(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source=NULL);
        void contentWindowRemoved(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source=NULL);
        void contentWindowMovedToFront(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source=NULL);

    protected:

        // optional: reference to DisplayGroupManager for non-DisplayGroupManager objects
        boost::weak_ptr<DisplayGroupManager> displayGroupManager_;

        // vector of all of its content windows
        std::vector<boost::shared_ptr<ContentWindow> > contentWindows_;
};

#endif
