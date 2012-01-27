#include "DisplayGroupInterface.h"
#include "DisplayGroupManager.h"
#include "ContentWindowManager.h"
#include "Content.h"
#include "main.h"

DisplayGroupInterface::DisplayGroupInterface(boost::shared_ptr<DisplayGroupManager> displayGroupManager)
{
    displayGroupManager_ = displayGroupManager;

    // copy all members from displayGroupManager
    if(displayGroupManager != NULL)
    {
        contentWindowManagers_ = displayGroupManager->contentWindowManagers_;
    }

    // needed for state saving / loading signals and slots
    qRegisterMetaType<std::string>("std::string");

    // connect signals from this to slots on the DisplayGroupManager
    // use queued connections for thread-safety
    connect(this, SIGNAL(contentWindowManagerAdded(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(addContentWindowManager(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(contentWindowManagerRemoved(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(removeContentWindowManager(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(contentWindowManagerMovedToFront(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(stateSaved(std::string, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(saveState(std::string, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(this, SIGNAL(stateLoaded(std::string, DisplayGroupInterface *)), displayGroupManager.get(), SLOT(loadState(std::string, DisplayGroupInterface *)), Qt::QueuedConnection);

    // connect signals on the DisplayGroupManager to slots on this
    // use queued connections for thread-safety
    connect(displayGroupManager.get(), SIGNAL(contentWindowManagerAdded(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), this, SLOT(addContentWindowManager(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroupManager.get(), SIGNAL(contentWindowManagerRemoved(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), this, SLOT(removeContentWindowManager(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroupManager.get(), SIGNAL(contentWindowManagerMovedToFront(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), this, SLOT(moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager>, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroupManager.get(), SIGNAL(stateSaved(std::string, DisplayGroupInterface *)), this, SLOT(saveState(std::string, DisplayGroupInterface *)), Qt::QueuedConnection);
    connect(displayGroupManager.get(), SIGNAL(stateLoaded(std::string, DisplayGroupInterface *)), this, SLOT(loadState(std::string, DisplayGroupInterface *)), Qt::QueuedConnection);

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

void DisplayGroupInterface::saveState(std::string filename, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    // only do the actual state saving if we're the DisplayGroupManager
    // we'll let the other DisplayGroupInterfaces know about the signal anyway though
    DisplayGroupManager * dgm = dynamic_cast<DisplayGroupManager *>(this);

    if(dgm != NULL)
    {
        dgm->saveStateXMLFile(filename);
    }

    if(source == NULL || dynamic_cast<DisplayGroupManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(stateSaved(filename, source));
    }
}

void DisplayGroupInterface::loadState(std::string filename, DisplayGroupInterface * source)
{
    if(source == this)
    {
        return;
    }

    // only do the actual state loading if we're the DisplayGroupManager
    // we'll let the other DisplayGroupInterfaces know about the signal anyway though
    DisplayGroupManager * dgm = dynamic_cast<DisplayGroupManager *>(this);

    if(dgm != NULL)
    {
        dgm->loadStateXMLFile(filename);
    }

    if(source == NULL || dynamic_cast<DisplayGroupManager *>(this) != NULL)
    {
        if(source == NULL)
        {
            source = this;
        }

        emit(stateLoaded(filename, source));
    }
}
