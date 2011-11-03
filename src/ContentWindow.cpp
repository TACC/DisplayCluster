#include "ContentWindow.h"
#include "Content.h"
#include "DisplayGroup.h"
#include "main.h"

qreal ContentWindow::zCounter_ = 0;

ContentWindow::ContentWindow()
{
    initialized_ = false;
}

ContentWindow::ContentWindow(boost::shared_ptr<Content> content)
{
    // defaults
    initialized_ = false;

    // default position / size
    x_ = y_ = 0.;
    w_ = h_ = 0.25;

    // default to centered
    centerX_ = 0.5;
    centerY_ = 0.5;

    // default to no zoom
    zoom_ = 1.;

    // default window state
    resizing_ = false;
    selected_ = false;

    // set content object
    content_ = content;
}

boost::shared_ptr<Content> ContentWindow::getContent()
{
    return content_;
}

boost::shared_ptr<DisplayGroup> ContentWindow::getDisplayGroup()
{
    return displayGroup_.lock();
}

void ContentWindow::setDisplayGroup(boost::shared_ptr<DisplayGroup> displayGroup)
{
    displayGroup_ = displayGroup;
}

void ContentWindow::setCoordinates(double x, double y, double w, double h)
{
    x_ = x;
    y_ = y;
    w_ = w;
    h_ = h;

    setRect(x_, y_, w_, h_);

    // force synchronization
    if(getDisplayGroup() != NULL)
    {
        getDisplayGroup()->sendDisplayGroup();
    }
}

void ContentWindow::getCoordinates(double &x, double &y, double &w, double &h)
{
    x = x_;
    y = y_;
    w = w_;
    h = h_;
}

void ContentWindow::setCenterCoordinates(double centerX, double centerY)
{
    centerX_ = centerX;
    centerY_ = centerY;

    // force synchronization
    if(getDisplayGroup() != NULL)
    {
        getDisplayGroup()->sendDisplayGroup();
    }
}

void ContentWindow::getCenterCoordinates(double &centerX, double &centerY)
{
    centerX = centerX_;
    centerY = centerY_;
}

void ContentWindow::setZoom(double zoom)
{
    zoom_ = zoom;

    // force synchronization
    if(getDisplayGroup() != NULL)
    {
        getDisplayGroup()->sendDisplayGroup();
    }
}

double ContentWindow::getZoom()
{
    return zoom_;
}

void ContentWindow::fixAspectRatio()
{
    int contentWidth, contentHeight;
    content_->getDimensions(contentWidth, contentHeight);

    double aspect = (double)contentWidth / (double)contentHeight;
    double screenAspect = (double)g_configuration->getTotalWidth() / (double)g_configuration->getTotalHeight();

    aspect /= screenAspect;

    if(aspect > w_ / h_)
    {
        h_ = w_ / aspect;
    }
    else if(aspect <= w_ / h_)
    {
        w_ = h_ * aspect;
    }

    QRectF r = rect();

    // the rect() isn't set until the first paint after serialization, so we need to make sure the (x,y) coordinates are correct
    // todo: this shouldn't be necessary, and should be fixed later...
    if(r.x() == 0. && r.y() == 0.)
    {
        r.setX(x_);
        r.setY(y_);
    }

    r.setWidth(w_);
    r.setHeight(h_);

    setRect(r);

    // setRect() won't cause an itemChange event, so trigger one manually
    itemChange(ItemPositionChange, 0);
}

void ContentWindow::render()
{
    content_->render(shared_from_this());

    // render the border
    double horizontalBorder = 5. / (double)g_configuration->getTotalHeight(); // 5 pixels
    double verticalBorder = (double)g_configuration->getTotalHeight() / (double)g_configuration->getTotalWidth() * horizontalBorder;

    glPushAttrib(GL_CURRENT_BIT);

    // color the border based on window state
    if(selected_ == true)
    {
        glColor4f(1,0,0,1);
    }
    else
    {
        glColor4f(1,1,1,1);
    }

    GLWindow::drawRectangle(x_-verticalBorder,y_-horizontalBorder,w_+2.*verticalBorder,h_+2.*horizontalBorder);

    glPopAttrib();
}

void ContentWindow::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    if(initialized_ == false)
    {
        // on first paint, initialize
        // note that we can't call some of these in the constructor, since they wouldn't then be called after de-serialization

        setRect(x_, y_, w_, h_);

        // graphics items are movable and fire events on geometry changes
        setFlag(QGraphicsItem::ItemIsMovable, true);
        setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

        // default fill color / opacity
        setBrush(QBrush(QColor(0, 0, 0, 128)));

        // default border
        setPen(QPen(QColor(0,0,0)));

        // new items at the front
        zCounter_ = zCounter_ + 1;
        setZValue(zCounter_);

        // force synchronization of display group since this is a new window
        getDisplayGroup()->sendDisplayGroup();

        initialized_ = true;
    }

    QGraphicsRectItem::paint(painter, option, widget);

    // default pen
    QPen pen;

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
    float verticalTextScale = 1. / (float)scene()->views()[0]->height();
    float horizontalTextScale = (float)scene()->views()[0]->height() / (float)scene()->views()[0]->width() * verticalTextScale;

    painter->scale(horizontalTextScale, verticalTextScale);

    QRectF textBoundingRect = QRectF(rect().x() / horizontalTextScale, rect().y() / verticalTextScale, rect().width() / horizontalTextScale, rect().height() / verticalTextScale);

    // get the label and render it
    QString label(content_->getURI().c_str());
    QString labelSection = label.section("/", -1, -1).prepend(" ");
    painter->drawText(textBoundingRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, labelSection);
}

void ContentWindow::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
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

                setRect(r);

                // setRect() won't cause an itemChange event, so trigger one manually
                itemChange(ItemPositionChange, 0);

                if(g_mainWindow->getConstrainAspectRatio() == true)
                {
                    fixAspectRatio();
                }
            }
            else
            {
                QPointF delta = event->pos() - event->lastPos();
                moveBy(delta.x(), delta.y());
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

            zoom_ *= (1. - zoomDelta);

            setZoom(zoom_);
        }
        else if(event->buttons().testFlag(Qt::LeftButton) == true)
        {
            // pan (move center coordinates)
            centerX_ += 2.*delta.x() / zoom_;
            centerY_ += 2.*delta.y() / zoom_;

            setCenterCoordinates(centerX_, centerY_);
        }
    }
}

void ContentWindow::mousePressEvent(QGraphicsSceneMouseEvent * event)
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
        getDisplayGroup()->removeContentWindow(shared_from_this());

        return;
    }

    // check to see if user clicked on the resize button
    if(fabs((r.x()+r.width()) - eventPos.x()) <= buttonWidth && fabs((r.y()+r.height()) - eventPos.y()) <= buttonHeight)
    {
        resizing_ = true;
    }

    // move content to front of display group
    getDisplayGroup()->moveContentWindowToFront(shared_from_this());

    // and to the front of the GUI display
    zCounter_ = zCounter_ + 1;
    setZValue(zCounter_);

    QGraphicsItem::mousePressEvent(event);
}

void ContentWindow::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
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

void ContentWindow::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    resizing_ = false;

    QGraphicsItem::mouseReleaseEvent(event);
}

QVariant ContentWindow::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if(change == ItemPositionChange)
    {
        QRectF r = mapRectToScene(rect());

        x_ = r.x() / scene()->width();
        y_ = r.y() / scene()->height();
        w_ = r.width() / scene()->width();
        h_ = r.height() / scene()->height();

        // note that we don't have to call sendDisplayGroup() here
        // the display group is already updated due to the marker (cursor) changing
    }

    return QGraphicsItem::itemChange(change, value);
}

void ContentWindow::getButtonDimensions(float &width, float &height)
{
    float sceneHeightFraction = 0.125;
    double screenAspect = (double)g_configuration->getTotalWidth() / (double)g_configuration->getTotalHeight();

    width = sceneHeightFraction / screenAspect;
    height = sceneHeightFraction;

    // clamp to half rect dimensions
    if(width > 0.5 * w_)
    {
        width = 0.49 * w_;
    }

    if(height > 0.5 * h_)
    {
        height = 0.49 * h_;
    }
}
