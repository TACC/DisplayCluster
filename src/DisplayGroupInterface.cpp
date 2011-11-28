#include "DisplayGroupInterface.h"
#include "DisplayGroupManager.h"
#include "ContentWindow.h"
#include "Content.h"

DisplayGroupInterface::DisplayGroupInterface(boost::shared_ptr<DisplayGroupManager> displayGroupManager)
{
    displayGroupManager_ = displayGroupManager;

    // copy all members from displayGroupManager
    if(displayGroupManager != NULL)
    {
        contentWindows_ = displayGroupManager->contentWindows_;
    }

    // connect signals from this to slots on the DisplayGroupManager
    // use queued connections for thread-safety
    connect(this, SIGNAL(contentWindowAdded(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(addContentWindow(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(contentWindowRemoved(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(removeContentWindow(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(contentWindowMovedToFront(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(moveContentWindowToFront(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), Qt::QueuedConnection);

    // connect signals on the DisplayGroupManager to slots on this
    // use queued connections for thread-safety
    connect(displayGroupManager.get(), SIGNAL(contentWindowAdded(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), this, SLOT(addContentWindow(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroupManager.get(), SIGNAL(contentWindowRemoved(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), this, SLOT(removeContentWindow(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroupManager.get(), SIGNAL(contentWindowMovedToFront(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), this, SLOT(moveContentWindowToFront(boost::shared_ptr<ContentWindow>, DisplayGroupInterface *)), Qt::QueuedConnection);

    // destruction
    connect(displayGroupManager.get(), SIGNAL(destroyed(QObject *)), this, SLOT(deleteLater()));
}

boost::shared_ptr<DisplayGroupManager> DisplayGroupInterface::getDisplayGroupManager()
{
    return displayGroupManager_.lock();
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

    if(source == NULL || dynamic_cast<DisplayGroupManager *>(this) != NULL)
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

    if(source == NULL || dynamic_cast<DisplayGroupManager *>(this) != NULL)
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

    if(source == NULL || dynamic_cast<DisplayGroupManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(contentWindowMovedToFront(contentWindow, source));
    }
}
