#include "ContentGraphicsItem.h"
#include "Content.h"
#include "DisplayGroup.h"

ContentGraphicsItem::ContentGraphicsItem(boost::shared_ptr<Content> parent)
{
    // defaults
    resizing_ = false;
    selected_ = false;

    parent_ = parent;

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    // default fill color / opacity
    setBrush(QBrush(QColor(0, 0, 0, 128)));

    // default border
    QPen pen;
    pen.setColor(QColor(0,0,0));
    pen.setWidthF(0.0025);

    setPen(pen);

    // default position / size
    setRect(0., 0., 0.25, 0.25);
}

void ContentGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    // handle mouse movements differently depending on selected mode of item
    if(selected_ == false)
    {
        // check to see if we're near enough a corner to do a resize event
        float cornerMoveFraction = 0.05;

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
            QPointF delta = event->pos() - event->lastPos();
            moveBy(delta.x(), delta.y());
        }
    }
    else
    {
        // handle zooms / pans
        QPointF delta = event->pos() - event->lastPos();

        if(button_ == Qt::RightButton)
        {
            // increment zoom
            if(boost::shared_ptr<Content> c = parent_.lock())
            {
                double zoom = c->getZoom();
                zoom *= (1. - delta.y());

                c->setZoom(zoom);
            }
        }
        else if(button_ == Qt::LeftButton)
        {
            // pan (move center coordinates)
            if(boost::shared_ptr<Content> c = parent_.lock())
            {
                double centerX, centerY;
                c->getCenterCoordinates(centerX, centerY);

                centerX += delta.x();
                centerY += delta.y();

                c->setCenterCoordinates(centerX, centerY);
            }
        }
    }
}

void ContentGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    button_ = event->button();

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

void ContentGraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
    selected_ = !selected_;

    // set the pen
    QPen p = pen();

    if(selected_ == true)
    {
        p.setColor(QColor(255,0,0));
    }
    else
    {
        p.setColor(QColor(0,0,0));
    }

    setPen(p);

    QGraphicsItem::mouseDoubleClickEvent(event);
}

void ContentGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    button_ = Qt::NoButton;

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
