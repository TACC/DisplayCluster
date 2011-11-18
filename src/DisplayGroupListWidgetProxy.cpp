#include "DisplayGroupListWidgetProxy.h"
#include "ContentWindow.h"
#include "Content.h"

DisplayGroupListWidgetProxy::DisplayGroupListWidgetProxy(boost::shared_ptr<DisplayGroup> displayGroup) : DisplayGroupInterface(displayGroup)
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

void DisplayGroupListWidgetProxy::addContentWindow(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source)
{
    DisplayGroupInterface::addContentWindow(contentWindow, source);

    if(source != this)
    {
        // for now, just clear and refresh the entire list, since this is just a read-only interface
        // later this could be modeled after DisplayGroupGraphicsViewProxy if we want to expand the interface
        refreshListWidget();
    }
}

void DisplayGroupListWidgetProxy::removeContentWindow(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source)
{
    DisplayGroupInterface::removeContentWindow(contentWindow, source);

    if(source != this)
    {
        refreshListWidget();
    }
}

void DisplayGroupListWidgetProxy::moveContentWindowToFront(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source)
{
    DisplayGroupInterface::moveContentWindowToFront(contentWindow, source);

    if(source != this)
    {
        refreshListWidget();
    }
}

void DisplayGroupListWidgetProxy::refreshListWidget()
{
    // clear list
    listWidget_->clear();

    for(unsigned int i=0; i<contentWindows_.size(); i++)
    {
        // add to list view
        QListWidgetItem * newItem = new QListWidgetItem(listWidget_);
        newItem->setText(contentWindows_[i]->getContent()->getURI().c_str());
    }
}
