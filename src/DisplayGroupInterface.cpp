#include "DisplayGroupInterface.h"
#include "DisplayGroup.h"
#include "ContentWindow.h"
#include "Content.h"

DisplayGroupInterface::DisplayGroupInterface(boost::shared_ptr<DisplayGroup> displayGroup)
{
    std::cerr << "DisplayGroupInterface ctor\n";
    displayGroup_ = displayGroup;

    // copy all members from displayGroup
    if(displayGroup != NULL)
    {
        contentWindows_ = displayGroup->contentWindows_;
    }

    // connect signals from this to slots on the DisplayGroup
    // use queued connections for thread-safety
    connect(this, SIGNAL(contentWindowAdded(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), displayGroup.get(), SLOT(addContentWindow(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(contentWindowRemoved(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), displayGroup.get(), SLOT(removeContentWindow(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(contentWindowMovedToFront(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), displayGroup.get(), SLOT(moveContentWindowToFront(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), Qt::QueuedConnection);

    // connect signals on the DisplayGroup to slots on this
    // use queued connections for thread-safety
    connect(displayGroup.get(), SIGNAL(contentWindowAdded(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), this, SLOT(addContentWindow(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroup.get(), SIGNAL(contentWindowRemoved(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), this, SLOT(removeContentWindow(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroup.get(), SIGNAL(contentWindowMovedToFront(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), this, SLOT(moveContentWindowToFront(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), Qt::QueuedConnection);

    // destruction
    connect(displayGroup.get(), SIGNAL(destroyed(QObject *)), this, SLOT(deleteLater()));
}

boost::shared_ptr<DisplayGroup> DisplayGroupInterface::getDisplayGroup()
{
    return displayGroup_.lock();
}

std::vector<boost::shared_ptr<ContentWindow> > DisplayGroupInterface::getContentWindows()
{
    return contentWindows_;
}

void DisplayGroupInterface::dump()
{
    std::cerr << "DisplayGroupInterface: " << std::hex << ((long)this) << "\n";
    for(unsigned int i=0; i<contentWindows_.size(); i++)
    {
        contentWindows_[i]->dump();
    }
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
