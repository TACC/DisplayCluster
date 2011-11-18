#include "DisplayGroupInterface.h"
#include "DisplayGroup.h"
#include "ContentWindow.h"
#include "Content.h"

DisplayGroupInterface::DisplayGroupInterface(boost::shared_ptr<DisplayGroup> displayGroup)
{
    displayGroup_ = displayGroup;

    // copy all members from displayGroup
    if(displayGroup != NULL)
    {
        contentWindows_ = displayGroup->contentWindows_;
    }
}

boost::shared_ptr<DisplayGroup> DisplayGroupInterface::getDisplayGroup()
{
    return displayGroup_.lock();
}

std::vector<boost::shared_ptr<ContentWindow> > DisplayGroupInterface::getContentWindows()
{
    return contentWindows_;
}

bool DisplayGroupInterface::hasContent(std::string uri)
{
    for(unsigned int i=0; i<contentWindows_.size(); i++)
    {
        if(contentWindows_[i]->getContent()->getURI() == uri)
        {
            return true;
        }
    }

    return false;
}

void DisplayGroupInterface::setContentWindows(std::vector<boost::shared_ptr<ContentWindow> > contentWindows)
{
    // remove existing content windows
    while(contentWindows_.size() > 0)
    {
        removeContentWindow(contentWindows_[0]);
    }

    // add new content windows
    for(unsigned int i=0; i<contentWindows.size(); i++)
    {
        addContentWindow(contentWindows[i]);
    }
}

void DisplayGroupInterface::addContentWindow(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    contentWindows_.push_back(contentWindow);

    if(source == NULL || dynamic_cast<DisplayGroup *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(contentWindowAdded(contentWindow, source));
    }
}

void DisplayGroupInterface::removeContentWindow(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    // find vector entry for content window
    std::vector<boost::shared_ptr<ContentWindow> >::iterator it;

    it = find(contentWindows_.begin(), contentWindows_.end(), contentWindow);

    if(it != contentWindows_.end())
    {
        // we found the entry
        // now, remove it
        contentWindows_.erase(it);
    }

    if(source == NULL || dynamic_cast<DisplayGroup *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(contentWindowRemoved(contentWindow, source));
    }
}

void DisplayGroupInterface::moveContentWindowToFront(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    // find vector entry for content window
    std::vector<boost::shared_ptr<ContentWindow> >::iterator it;

    it = find(contentWindows_.begin(), contentWindows_.end(), contentWindow);

    if(it != contentWindows_.end())
    {
        // we found the entry
        // now, move it to end of the list (last item rendered is on top)
        contentWindows_.erase(it);
        contentWindows_.push_back(contentWindow);
    }

    if(source == NULL || dynamic_cast<DisplayGroup *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(contentWindowMovedToFront(contentWindow, source));
    }
}
