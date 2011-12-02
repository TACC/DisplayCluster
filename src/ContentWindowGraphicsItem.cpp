#include "ContentWindowGraphicsItem.h"
#include "Content.h"
#include "ContentWindowManager.h"
#include "DisplayGroupManager.h"
#include "DisplayGroupGraphicsView.h"
#include "main.h"

qreal ContentWindowGraphicsItem::zCounter_ = 0;

ContentWindowGraphicsItem::ContentWindowGraphicsItem(boost::shared_ptr<ContentWindowManager> contentWindowManager) : ContentWindowInterface(contentWindowManager)
{
    // defaults
    resizing_ = false;

    // graphics items are movable and fire events on geometry changes
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    // default fill color / opacity
    setBrush(QBrush(QColor(0, 0, 0, 128)));

    // border based on if we're selected or not
    // use the -1 argument to force an update but not emit signals
    setSelected(selected_, (ContentWindowInterface *)-1);

    // current coordinates
    setRect(x_, y_, w_, h_);

    // new items at the front
    // we assume that interface items will be constructed in depth order so this produces the correct result...
    setZToFront();
}

void ContentWindowGraphicsItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    QGraphicsRectItem::paint(painter, option, widget);

    boost::shared_ptr<ContentWindowManager> contentWindowManager = getContentWindowManager();

    if(contentWindowManager != NULL)
    {
        // default pen
        QPen pen;

        // button dimensions
        float buttonWidth, buttonHeight;
        contentWindowManager->getButtonDimensions(buttonWidth, buttonHeight);

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

        // text label

        // set the font
        float fontSize = 24.;

        QFont font;
        font.setPixelSize(fontSize);
        painter->setFont(font);

        // color the text black
        pen.setColor(QColor(0,0,0));
        painter->setPen(pen);

        // scale the text size down to the height of the graphics view
        // and, calculate the bounding rectangle for the text based on this scale
        // the dimensions of the view need to be corrected for the tiled display aspect ratio
        // recall the tiled display UI is only part of the graphics view since we show it at the correct aspect ratio
        float viewWidth = (float)scene()->views()[0]->width();
        float viewHeight = (float)scene()->views()[0]->height();

        float tiledDisplayAspect = (float)g_configuration->getTotalWidth() / (float)g_configuration->getTotalHeight();

        if(viewWidth / viewHeight > tiledDisplayAspect)
        {
            viewWidth = viewHeight * tiledDisplayAspect;
        }
        else if(viewWidth / viewHeight <= tiledDisplayAspect)
        {
            viewHeight = viewWidth / tiledDisplayAspect;
        }

        float verticalTextScale = 1. / viewHeight;
        float horizontalTextScale = viewHeight / viewWidth * verticalTextScale;

        painter->scale(horizontalTextScale, verticalTextScale);

        QRectF textBoundingRect = QRectF(rect().x() / horizontalTextScale, rect().y() / verticalTextScale, rect().width() / horizontalTextScale, rect().height() / verticalTextScale);

        // get the label and render it
        QString label(contentWindowManager->getContent()->getURI().c_str());
        QString labelSection = label.section("/", -1, -1).prepend(" ");
        painter->drawText(textBoundingRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, labelSection);
    }
}

void ContentWindowGraphicsItem::setCoordinates(double x, double y, double w, double h, ContentWindowInterface * source)
{
    ContentWindowInterface::setCoordinates(x, y, w, h, source);

    if(source != this)
    {
        setPos(x_, y_);
        setRect(mapRectFromScene(x_, y_, w_, h_));
    }
}

void ContentWindowGraphicsItem::setPosition(double x, double y, ContentWindowInterface * source)
{
    ContentWindowInterface::setPosition(x, y, source);

    if(source != this)
    {
        setPos(x_, y_);
        setRect(mapRectFromScene(x_, y_, w_, h_));
    }
}

void ContentWindowGraphicsItem::setSize(double w, double h, ContentWindowInterface * source)
{
    ContentWindowInterface::setSize(w, h, source);

    if(source != this)
    {
        setPos(x_, y_);
        setRect(mapRectFromScene(x_, y_, w_, h_));
    }
}

void ContentWindowGraphicsItem::setSelected(bool selected, ContentWindowInterface * source)
{
    ContentWindowInterface::setSelected(selected, source);

    if(source != this)
    {
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

        // force a redraw
        update();
    }
}

void ContentWindowGraphicsItem::setZToFront()
{
    zCounter_ = zCounter_ + 1;
    setZValue(zCounter_);
}

void ContentWindowGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    // handle mouse movements differently depending on selected mode of item
    if(selected_ == false)
    {
        if(event->buttons().testFlag(Qt::LeftButton) == true)
        {
            if(resizing_ == true)
            {
                QRectF r = rect();
                QPointF eventPos = event->pos();

                r.setBottomRight(eventPos);

                QRectF sceneRect = mapRectToScene(r);

                double w = sceneRect.width();
                double h = sceneRect.height();

                setSize(w, h);
            }
            else
            {
                QPointF delta = event->pos() - event->lastPos();

                double x = x_ + delta.x();
                double y = y_ + delta.y();

                setPosition(x, y);
            }
        }
    }
    else
    {
        // handle zooms / pans
        QPointF delta = event->scenePos() - event->lastScenePos();

        if(event->buttons().testFlag(Qt::RightButton) == true)
        {
            // increment zoom

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

            double zoom = zoom_ * (1. - zoomDelta);

            setZoom(zoom);
        }
        else if(event->buttons().testFlag(Qt::LeftButton) == true)
        {
            // pan (move center coordinates)
            double centerX = centerX_ + 2.*delta.x() / zoom_;
            double centerY = centerY_ + 2.*delta.y() / zoom_;

            setCenter(centerX, centerY);
        }
    }
}

void ContentWindowGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    boost::shared_ptr<ContentWindowManager> contentWindowManager = getContentWindowManager();

    if(contentWindowManager != NULL)
    {
        // button dimensions
        float buttonWidth, buttonHeight;
        contentWindowManager->getButtonDimensions(buttonWidth, buttonHeight);

        // item rectangle and event position
        QRectF r = rect();
        QPointF eventPos = event->pos();

        // check to see if user clicked on the close button
        if(fabs((r.x()+r.width()) - eventPos.x()) <= buttonWidth && fabs((r.y()) - eventPos.y()) <= buttonHeight)
        {
            close();

            return;
        }

        // check to see if user clicked on the resize button
        if(fabs((r.x()+r.width()) - eventPos.x()) <= buttonWidth && fabs((r.y()+r.height()) - eventPos.y()) <= buttonHeight)
        {
            resizing_ = true;
        }
    }

    // move to the front of the GUI display
    moveToFront();

    QGraphicsItem::mousePressEvent(event);
}

void ContentWindowGraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
    bool selected = !selected_;

    setSelected(selected);

    QGraphicsItem::mouseDoubleClickEvent(event);
}

void ContentWindowGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    resizing_ = false;

    QGraphicsItem::mouseReleaseEvent(event);
}
