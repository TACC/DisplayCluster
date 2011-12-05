#ifndef DISPLAY_GROUP_JOYSTICK_H
#define DISPLAY_GROUP_JOYSTICK_H

#include "DisplayGroupInterface.h"

class Marker;
class ContentWindowInterface;

class DisplayGroupJoystick : public DisplayGroupInterface {

    public:

        DisplayGroupJoystick(boost::shared_ptr<DisplayGroupManager> displayGroupManager);

        // re-implemented DisplayGroupInterface slots
        void addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);

        boost::shared_ptr<Marker> getMarker();
        std::vector<boost::shared_ptr<ContentWindowInterface> > getContentWindowInterfaces();

        boost::shared_ptr<ContentWindowInterface> getContentWindowInterfaceUnderMarker();

    private:

        boost::shared_ptr<Marker> marker_;
        std::vector<boost::shared_ptr<ContentWindowInterface> > contentWindowInterfaces_;
};

#endif
