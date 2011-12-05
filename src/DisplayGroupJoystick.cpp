#include "DisplayGroupJoystick.h"
#include "DisplayGroupManager.h"
#include "ContentWindowInterface.h"
#include "ContentWindowManager.h"
#include "Marker.h"

DisplayGroupJoystick::DisplayGroupJoystick(boost::shared_ptr<DisplayGroupManager> displayGroupManager) : DisplayGroupInterface(displayGroupManager)
{
    marker_ = displayGroupManager->getNewMarker();
}

void DisplayGroupJoystick::addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::addContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        boost::shared_ptr<ContentWindowInterface> cwi(new ContentWindowInterface(contentWindowManager));
        contentWindowInterfaces_.push_back(cwi);
    }
}

void DisplayGroupJoystick::removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::removeContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        for(unsigned int i=0; i<contentWindowInterfaces_.size(); i++)
        {
            if(contentWindowInterfaces_[i]->getContentWindowManager() == contentWindowManager)
            {
                contentWindowInterfaces_.erase(contentWindowInterfaces_.begin() + i);
                return;
            }
        }
    }
}

void DisplayGroupJoystick::moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::moveContentWindowManagerToFront(contentWindowManager, source);

    if(source != this)
    {
        for(unsigned int i=0; i<contentWindowInterfaces_.size(); i++)
        {
            if(contentWindowInterfaces_[i]->getContentWindowManager() == contentWindowManager)
            {
                boost::shared_ptr<ContentWindowInterface> cwi = contentWindowInterfaces_[i];
                contentWindowInterfaces_.erase(contentWindowInterfaces_.begin() + i);
                contentWindowInterfaces_.push_back(cwi);
                return;
            }
        }
    }
}

boost::shared_ptr<Marker> DisplayGroupJoystick::getMarker()
{
    return marker_;
}

std::vector<boost::shared_ptr<ContentWindowInterface> > DisplayGroupJoystick::getContentWindowInterfaces()
{
    return contentWindowInterfaces_;
}

boost::shared_ptr<ContentWindowInterface> DisplayGroupJoystick::getContentWindowInterfaceUnderMarker()
{
    // find the last item in our vector underneath the marker
    float markerX, markerY;
    marker_->getPosition(markerX, markerY);

    // need signed int here
    for(int i=(int)contentWindowInterfaces_.size()-1; i>=0; i--)
    {
        // get rectangle
        double x,y,w,h;
        contentWindowInterfaces_[i]->getCoordinates(x,y,w,h);

        if(QRectF(x,y,w,h).contains(markerX, markerY) == true)
        {
            return contentWindowInterfaces_[i];
        }
    }

    return boost::shared_ptr<ContentWindowInterface>();
}
