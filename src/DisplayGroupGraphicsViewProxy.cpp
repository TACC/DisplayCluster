#include "DisplayGroupGraphicsViewProxy.h"
#include "DisplayGroupGraphicsView.h"
#include "ContentWindow.h"
#include "ContentWindowGraphicsItem.h"

DisplayGroupGraphicsViewProxy::DisplayGroupGraphicsViewProxy(boost::shared_ptr<DisplayGroupManager> displayGroupManager) : DisplayGroupInterface(displayGroupManager)
{
    // create actual graphics view
    graphicsView_ = new DisplayGroupGraphicsView();
}

DisplayGroupGraphicsViewProxy::~DisplayGroupGraphicsViewProxy()
{
    delete graphicsView_;
}

DisplayGroupGraphicsView * DisplayGroupGraphicsViewProxy::getGraphicsView()
{
    return graphicsView_;
}

void DisplayGroupGraphicsViewProxy::addContentWindow(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source)
{
    DisplayGroupInterface::addContentWindow(contentWindow, source);

    if(source != this)
    {
        ContentWindowGraphicsItem * cwgi = new ContentWindowGraphicsItem(contentWindow);
        graphicsView_->scene()->addItem((QGraphicsItem *)cwgi);
    }
}

void DisplayGroupGraphicsViewProxy::removeContentWindow(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source)
{
    DisplayGroupInterface::removeContentWindow(contentWindow, source);

    if(source != this)
    {
        // find ContentWindowGraphicsItem associated with contentWindow
        QList<QGraphicsItem *> itemsList = graphicsView_->scene()->items();

        for(int i=0; i<itemsList.size(); i++)
        {
            // need dynamic cast to make sure this is actually a CWGI
            ContentWindowGraphicsItem * cwgi = dynamic_cast<ContentWindowGraphicsItem *>(itemsList.at(i));

            if(cwgi != NULL && cwgi->getContentWindow() == contentWindow)
            {
                graphicsView_->scene()->removeItem(itemsList.at(i));
            }
        }
    }
}

void DisplayGroupGraphicsViewProxy::moveContentWindowToFront(boost::shared_ptr<ContentWindow> contentWindow, DisplayGroupInterface * source)
{
    DisplayGroupInterface::moveContentWindowToFront(contentWindow, source);

    if(source != this)
    {
        // find ContentWindowGraphicsItem associated with contentWindow
        QList<QGraphicsItem *> itemsList = graphicsView_->scene()->items();

        for(int i=0; i<itemsList.size(); i++)
        {
            // need dynamic cast to make sure this is actually a CWGI
            ContentWindowGraphicsItem * cwgi = dynamic_cast<ContentWindowGraphicsItem *>(itemsList.at(i));

            if(cwgi != NULL && cwgi->getContentWindow() == contentWindow)
            {
                // don't call cwgi->moveToFront() here or that'll lead to infinite recursion!
                cwgi->setZToFront();
            }
        }
    }
}
