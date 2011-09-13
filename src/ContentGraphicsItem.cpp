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

void ContentGraphicsItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    QGraphicsRectItem::paint(painter, option, widget);

    // default pen
    QPen pen;
    pen.setWidthF(0.0025);

    // button dimensions
    float buttonWidth, buttonHeight;
    getButtonDimensions(buttonWidth, buttonHeight);

    // draw close button
    QRectF closeRect(rect().x() + rect().width() - buttonWidth, rect().y(), buttonWidth, buttonHeight);
    pen.setColor(QColor(255,0,0));
    painter->setPen(pen);
    painter->drawRect(closeRect);
    painter->drawLine(QPointF(rect().x() + rect().width() - buttonWidth, rect().y()), QPointF(rect().x() + rect().width(), rect().y() + buttonHeight));
    painter->drawLine(QPointF(rect().x() + rect().width(), rect().y()), QPointF(rect().x() + rect().width() - buttonWidth, rect().y() + buttonHeight));

    // resize indicator
    QRectF resizeRect(rect().x() + rect().width() - buttonWidth, rect().y() + rect().height() - buttonHeight, buttonWidth, buttonHeight);
    pen.setColor(QColor(128,128,128));
    painter->setPen(pen);
    painter->drawRect(resizeRect);
    painter->drawLine(QPointF(rect().x() + rect().width(), rect().y() + rect().height() - buttonHeight), QPointF(rect().x() + rect().width() - buttonWidth, rect().y() + rect().height()));
}

void ContentGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    // handle mouse movements differently depending on selected mode of item
    if(selected_ == false)
    {
        if(resizing_ == true)
        {
            QRectF r = rect();
            QPointF eventPos = event->pos();

            r.setBottomRight(eventPos);

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
        QPointF delta = event->scenePos() - event->lastScenePos();

        if(button_ == Qt::RightButton)
        {
            // increment zoom
            if(boost::shared_ptr<Content> c = parent_.lock())
            {
                double zoom = c->getZoom();

                // if this is a touch event, use cross-product for determining change in zoom (counterclockwise rotation == zoom in, etc.)
                // otherwise, use y as the change in zoom
                double zoomDelta;

                if(event->modifiers().testFlag(Qt::AltModifier) == true)
                {
                    zoomDelta = (event->scenePos().x()-0.5) * delta.y() - (event->scenePos().y()-0.5) * delta.x();
                    zoomDelta *= 2.;
                }
                else
                {
                    zoomDelta = delta.y();
                }

                zoom *= (1. - zoomDelta);

                c->setZoom(zoom);
            }
        }
        else if(button_ == Qt::LeftButton)
        {
            // pan (move center coordinates)
            if(boost::shared_ptr<Content> c = parent_.lock())
            {
                double zoom = c->getZoom();

                double centerX, centerY;
                c->getCenterCoordinates(centerX, centerY);

                centerX += 2.*delta.x() / zoom;
                centerY += 2.*delta.y() / zoom;

                c->setCenterCoordinates(centerX, centerY);
            }
        }
    }
}

void ContentGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    // button dimensions
    float buttonWidth, buttonHeight;
    getButtonDimensions(buttonWidth, buttonHeight);

    // item rectangle and event position
    QRectF r = rect();
    QPointF eventPos = event->pos();

    // check to see if user clicked on the close button
    if(fabs((r.x()+r.width()) - eventPos.x()) <= buttonWidth && fabs((r.y()) - eventPos.y()) <= buttonHeight)
    {
        if(boost::shared_ptr<Content> c = parent_.lock())
        {
            if(boost::shared_ptr<DisplayGroup> dg = c->getDisplayGroup())
            {
                dg->removeContent(c);
            }
        }

        return;
    }

    // check to see if user clicked on the resize button
    if(fabs((r.x()+r.width()) - eventPos.x()) <= buttonWidth && fabs((r.y()+r.height()) - eventPos.y()) <= buttonHeight)
    {
        resizing_ = true;
    }

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

void ContentGraphicsItem::getButtonDimensions(float &width, float &height)
{
    float sceneFraction = 0.05;

    width = sceneFraction * scene()->height();
    height = sceneFraction * scene()->height();

    // clamp to rect dimensions
    if(width > 0.5 * rect().width())
    {
        width = 0.49 * rect().width();
    }

    if(height > 0.5 * rect().height())
    {
        height = 0.49 * rect().height();
    }
}
