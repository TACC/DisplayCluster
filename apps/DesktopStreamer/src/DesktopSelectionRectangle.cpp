#include "DesktopSelectionRectangle.h"
#include "main.h"

DesktopSelectionRectangle::DesktopSelectionRectangle()
{
    // defaults
    resizing_ = false;

    // current coordinates from MainWindow
    g_mainWindow->getCoordinates(x_, y_, width_, height_);

    // graphics items are movable
    setFlag(QGraphicsItem::ItemIsMovable, true);

    // default pen
    setPen(QPen(QBrush(QColor(255, 0, 0)), PEN_WIDTH));

    // current coordinates, accounting for width of the pen outline
    setRect(x_-PEN_WIDTH/2, y_-PEN_WIDTH/2, width_+PEN_WIDTH, height_+PEN_WIDTH);
}

void DesktopSelectionRectangle::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    QGraphicsRectItem::paint(painter, option, widget);
}

void DesktopSelectionRectangle::setCoordinates(int x, int y, int width, int height)
{
    x_ = x;
    y_ = y;
    width_ = width;
    height_ = height;

    setRect(mapRectFromScene(x_-PEN_WIDTH/2, y_-PEN_WIDTH/2, width_+PEN_WIDTH, height_+PEN_WIDTH));
}

void DesktopSelectionRectangle::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    if(event->buttons().testFlag(Qt::LeftButton) == true)
    {
        if(resizing_ == true)
        {
            QRectF r = rect();
            QPointF eventPos = event->pos();

            r.setBottomRight(eventPos);

            setRect(r);
        }
        else
        {
            QPointF delta = event->pos() - event->lastPos();

            moveBy(delta.x(), delta.y());
        }

        updateCoordinates();
    }
}

void DesktopSelectionRectangle::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    // item rectangle and event position
    QRectF r = rect();
    QPointF eventPos = event->pos();

    // check to see if user clicked on the resize button
    if(fabs((r.x()+r.width()) - eventPos.x()) <= CORNER_RESIZE_THRESHHOLD && fabs((r.y()+r.height()) - eventPos.y()) <= CORNER_RESIZE_THRESHHOLD)
    {
        resizing_ = true;
    }

    QGraphicsItem::mousePressEvent(event);
}

void DesktopSelectionRectangle::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    resizing_ = false;

    QGraphicsItem::mouseReleaseEvent(event);
}

void DesktopSelectionRectangle::updateCoordinates()
{
    QRectF sceneRect = mapRectToScene(rect());

    x_ = (int)sceneRect.x() + PEN_WIDTH/2;
    y_ = (int)sceneRect.y() + PEN_WIDTH/2;
    width_ = (int)sceneRect.width() - PEN_WIDTH;
    height_ = (int)sceneRect.height() - PEN_WIDTH;

    g_mainWindow->setCoordinates(x_, y_, width_, height_);
}
