#include "ContentGraphicsItem.h"
#include "Content.h"
#include "DisplayGroup.h"

ContentGraphicsItem::ContentGraphicsItem(boost::shared_ptr<Content> parent)
{
    // defaults
    resizing_ = false;

    parent_ = parent;

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setRect(0., 0., 0.25, 0.25);
}

void ContentGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    float cornerMoveFraction = 0.05;

    // check to see if we're near enough a corner to do a resize event
    QRectF r = rect();

    QPointF eventPos = event->pos();

    if(QLineF(r.bottomLeft(), eventPos).length() < cornerMoveFraction * scene()->width())
    {
        resizing_ = true;
    }

    if(resizing_ == true)
    {       
        r.setBottomLeft(eventPos);

        setRect(r);

        // update coordinates of parent during resize
        updateParent();
    }
    else
    {
        QGraphicsItem::mouseMoveEvent(event);
    }
}

void ContentGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    // move content to front of display group
    if(boost::shared_ptr<Content> c = parent_.lock())
    {
        if(boost::shared_ptr<DisplayGroup> dg = c->getDisplayGroup())
        {
            // todo: should this be moved to Content::?
            dg->moveContentToFront(c);

            // update coordinates of parent (depth change)
            updateParent();
        }
    }

    QGraphicsItem::mousePressEvent(event);
}

void ContentGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    resizing_ = false;

    QGraphicsItem::mouseReleaseEvent(event);
}

QVariant ContentGraphicsItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if(change == ItemPositionChange)
    {
        updateParent();
    }

    return QGraphicsItem::itemChange(change, value);
}

void ContentGraphicsItem::updateParent()
{
    QRectF r = mapRectToScene(rect());

    if(boost::shared_ptr<Content> c = parent_.lock())
    {
        c->setCoordinates(r.x() / scene()->width(), r.y() / scene()->height(), r.width() / scene()->width(), r.height() / scene()->height());
    }
}
