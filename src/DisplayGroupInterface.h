#ifndef DISPLAY_GROUP_INTERFACE_H
#define DISPLAY_GROUP_INTERFACE_H

#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

class DisplayGroupManager;
class ContentWindowManager;

class DisplayGroupInterface : public QObject {
    Q_OBJECT

    public:

        DisplayGroupInterface() { }
        DisplayGroupInterface(boost::shared_ptr<DisplayGroupManager> displayGroupManager);

        boost::shared_ptr<DisplayGroupManager> getDisplayGroupManager();

        std::vector<boost::shared_ptr<ContentWindowManager> > getContentWindowManagers();
        bool hasContent(std::string uri);
        boost::shared_ptr<ContentWindowManager> getContentWindowManager(std::string uri);

        // remove all current ContentWindowManagers and add the vector of provided ContentWindowManagers
        void setContentWindowManagers(std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers);

    public slots:

        // these methods set the local copies of the state variables if source != this
        // they will emit signals if source == NULL or if this is a DisplayGroup object
        // the source argument should not be provided by users -- only by these functions
        virtual void addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        virtual void removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        virtual void moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);

    signals:

        // emitting these signals will trigger updates on the corresponding DisplayGroup
        // as well as all other DisplayGroupInterfaces to that DisplayGroup
        void contentWindowManagerAdded(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void contentWindowManagerRemoved(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void contentWindowManagerMovedToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);

    protected:

        // optional: reference to DisplayGroupManager for non-DisplayGroupManager objects
        boost::weak_ptr<DisplayGroupManager> displayGroupManager_;

        // vector of all of its content window managers
        std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers_;
};

#endif
