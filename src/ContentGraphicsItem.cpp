#include "ContentGraphicsItem.h"
#include "Content.h"

ContentGraphicsItem::ContentGraphicsItem(Content * parent)
{
    // defaults
    resizing_ = false;

    parent_ = parent;

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    setRect(0., 0., DEFAULT_SIZE, DEFAULT_SIZE);
}

void ContentGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    // check to see if we're near enough a corner to do a resize event
    QRectF r = rect();

    QPointF eventPos = event->pos();

    if(QLineF(r.bottomLeft(), eventPos).length() < CORNER_MOVE_FRACTION * scene()->width())
    {
        resizing_ = true;
    }

    if(resizing_ == true)
    {       
        r.setBottomLeft(eventPos);

        setRect(r);

        // update coordinates of parent during resize
        QRectF rScene = mapRectToScene(rect());
        parent_->setCoordinates(rScene.x() / scene()->width(), rScene.y() / scene()->height(), rScene.width() / scene()->width(), rScene.height() / scene()->height());

        update();
    }
    else
    {
        QGraphicsItem::mouseMoveEvent(event);
    }
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
        QRectF r = mapRectToScene(rect());

        parent_->setCoordinates(r.x() / scene()->width(), r.y() / scene()->height(), r.width() / scene()->width(), r.height() / scene()->height());
    }

    return QGraphicsItem::itemChange(change, value);
}
