#include "DisplayGroupGraphicsViewProxy.h"
#include "DisplayGroupGraphicsView.h"
#include "ContentWindowManager.h"
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

void DisplayGroupGraphicsViewProxy::addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::addContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        ContentWindowGraphicsItem * cwgi = new ContentWindowGraphicsItem(contentWindowManager);
        graphicsView_->scene()->addItem((QGraphicsItem *)cwgi);
    }
}

void DisplayGroupGraphicsViewProxy::removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::removeContentWindowManager(contentWindowManager, source);

    if(source != this)
    {
        // find ContentWindowGraphicsItem associated with contentWindowManager
        QList<QGraphicsItem *> itemsList = graphicsView_->scene()->items();

        for(int i=0; i<itemsList.size(); i++)
        {
            // need dynamic cast to make sure this is actually a CWGI
            ContentWindowGraphicsItem * cwgi = dynamic_cast<ContentWindowGraphicsItem *>(itemsList.at(i));

            if(cwgi != NULL && cwgi->getContentWindowManager() == contentWindowManager)
            {
                graphicsView_->scene()->removeItem(itemsList.at(i));
            }
        }
    }
}

void DisplayGroupGraphicsViewProxy::moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source)
{
    DisplayGroupInterface::moveContentWindowManagerToFront(contentWindowManager, source);

    if(source != this)
    {
        // find ContentWindowGraphicsItem associated with contentWindowManager
        QList<QGraphicsItem *> itemsList = graphicsView_->scene()->items();

        for(int i=0; i<itemsList.size(); i++)
        {
            // need dynamic cast to make sure this is actually a CWGI
            ContentWindowGraphicsItem * cwgi = dynamic_cast<ContentWindowGraphicsItem *>(itemsList.at(i));

            if(cwgi != NULL && cwgi->getContentWindowManager() == contentWindowManager)
            {
                // don't call cwgi->moveToFront() here or that'll lead to infinite recursion!
                cwgi->setZToFront();
            }
        }
    }
}
