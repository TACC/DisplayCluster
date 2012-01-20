#include "DisplayGroupInterface.h"
#include "DisplayGroupManager.h"
#include "ContentWindowManager.h"
#include "Content.h"

DisplayGroupInterface::DisplayGroupInterface(boost::shared_ptr<DisplayGroupManager> displayGroupManager)
{
    displayGroupManager_ = displayGroupManager;

    // copy all members from displayGroupManager
    if(displayGroupManager != NULL)
    {
        contentWindowManagers_ = displayGroupManager->contentWindowManagers_;
    }

    // connect signals from this to slots on the DisplayGroupManager
    // use queued connections for thread-safety
    connect(this, SIGNAL(contentWindowManagerAdded(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(addContentWindowManager(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(contentWindowManagerRemoved(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(removeContentWindowManager(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(contentWindowManagerMovedToFront(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);

    // connect signals on the DisplayGroupManager to slots on this
    // use queued connections for thread-safety
    connect(displayGroupManager.get(), SIGNAL(contentWindowManagerAdded(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), this, SLOT(addContentWindowManager(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroupManager.get(), SIGNAL(contentWindowManagerRemoved(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), this, SLOT(removeContentWindowManager(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroupManager.get(), SIGNAL(contentWindowManagerMovedToFront(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), this, SLOT(moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);

    // destruction
    connect(displayGroupManager.get(), SIGNAL(destroyed(QObject *)), this, SLOT(deleteLater()));
}

boost::shared_ptr<DisplayGroupManager> DisplayGroupInterface::getDisplayGroupManager()
{
    return displayGroupManager_.lock();
}

std::vector<boost::shared_ptr<ContentWindowManager> > DisplayGroupInterface::getContentWindowManagers()
{
    return contentWindowManagers_;
}

bool DisplayGroupInterface::hasContent(std::string uri)
{
    for(unsigned int i=0; i<contentWindowManagers_.size(); i++)
    {
        if(contentWindowManagers_[i]->getContent()->getURI() == uri)
        {
            return true;
        }
    }

    return false;
}

boost::shared_ptr<ContentWindowManager> DisplayGroupInterface::getContentWindowManager(std::string uri)
{
    for(unsigned int i=0; i<contentWindowManagers_.size(); i++)
    {
        if(contentWindowManagers_[i]->getContent()->getURI() == uri)
        {
            return contentWindowManagers_[i];
        }
    }

    return boost::shared_ptr<ContentWindowManager>();
}

void DisplayGroupInterface::setContentWindowManagers(std::vector<boost::shared_ptr<ContentWindowManager> > contentWindowManagers)
{
    // remove existing content window managers
    while(contentWindowManagers_.size() > 0)
    {
        removeContentWindowManager(contentWindowManagers_[0]);
    }

    // add new content window managers
    for(unsigned int i=0; i<contentWindowManagers.size(); i++)
    {
        addContentWindowManager(contentWindowManagers[i]);
    }
}

void DisplayGroupInterface::addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    contentWindowManagers_.push_back(contentWindowManager);

    if(source == NULL || dynamic_cast<DisplayGroupManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(contentWindowManagerAdded(contentWindowManager, source));
    }
}

void DisplayGroupInterface::removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    // find vector entry for content window manager
    std::vector<boost::shared_ptr<ContentWindowManager> >::iterator it;

    it = find(contentWindowManagers_.begin(), contentWindowManagers_.end(), contentWindowManager);

    if(it != contentWindowManagers_.end())
    {
        // we found the entry
        // now, remove it
        contentWindowManagers_.erase(it);
    }

    if(source == NULL || dynamic_cast<DisplayGroupManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(contentWindowManagerRemoved(contentWindowManager, source));
    }
}

void DisplayGroupInterface::moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    // find vector entry for content window manager
    std::vector<boost::shared_ptr<ContentWindowManager> >::iterator it;

    it = find(contentWindowManagers_.begin(), contentWindowManagers_.end(), contentWindowManager);

    if(it != contentWindowManagers_.end())
    {
        // we found the entry
        // now, move it to end of the list (last item rendered is on top)
        contentWindowManagers_.erase(it);
        contentWindowManagers_.push_back(contentWindowManager);
    }

    if(source == NULL || dynamic_cast<DisplayGroupManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(contentWindowManagerMovedToFront(contentWindowManager, source));
    }
}
