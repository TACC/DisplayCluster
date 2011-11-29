#include "DisplayGroupListWidgetProxy.h"
#include "ContentWindowManager.h"
#include "Content.h"

DisplayGroupListWidgetProxy::DisplayGroupListWidgetProxy(boost::shared_ptr<DisplayGroupManager> displayGroupManager) : DisplayGroupInterface(displayGroupManager)
{
    // create actual list widget
    listWidget_ = new QListWidget();
}

DisplayGroupListWidgetProxy::~DisplayGroupListWidgetProxy()
{
    delete listWidget_;
}

QListWidget * DisplayGroupListWidgetProxy::getListWidget()
{
    return listWidget_;
}

void DisplayGroupListWidgetProxy::addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::addContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        // for now, just clear and refresh the entire list, since this is just a read-only interface
        // later this could be modeled after DisplayGroupGraphicsViewProxy if we want to expand the interface
        refreshListWidget();
    }
}

void DisplayGroupListWidgetProxy::removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::removeContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        refreshListWidget();
    }
}

void DisplayGroupListWidgetProxy::moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::moveContentWindowManagerToFront(contentWindowManager, source);

    if(source != this)
    {
        refreshListWidget();
    }
}

void DisplayGroupListWidgetProxy::refreshListWidget()
{
    // clear list
    listWidget_->clear();

    for(unsigned int i=0; i<contentWindowManagers_.size(); i++)
    {
        // add to list view
        QListWidgetItem * newItem = new QListWidgetItem(listWidget_);
        newItem->setText(contentWindowManagers_[i]->getContent()->getURI().c_str());
    }
}
