/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

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

    // graphics items are movable
    setFlag(QGraphicsItem::ItemIsMovable, true);

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
        painter->drawText(textBoundingRect, Qt::AlignLeft | Qt::AlignTop, labelSection);

        // draw window info at smaller scale
        verticalTextScale *= 0.5;
        horizontalTextScale *= 0.5;

        painter->scale(0.5, 0.5);

        textBoundingRect = QRectF(rect().x() / horizontalTextScale, rect().y() / verticalTextScale, rect().width() / horizontalTextScale, rect().height() / verticalTextScale);

        QString coordinatesLabel = QString(" (") + QString::number(x_, 'f', 2) + QString(" ,") + QString::number(y_, 'f', 2) + QString(", ") + QString::number(w_, 'f', 2) + QString(", ") + QString::number(h_, 'f', 2) + QString(")\n");
        QString zoomCenterLabel = QString(" zoom = ") + QString::number(zoom_, 'f', 2) + QString(" @ (") + QString::number(centerX_, 'f', 2) + QString(", ") + QString::number(centerY_, 'f', 2) + QString(")");

        QString windowInfoLabel = coordinatesLabel + zoomCenterLabel;
        painter->drawText(textBoundingRect, Qt::AlignLeft | Qt::AlignBottom, windowInfoLabel);
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

void ContentWindowGraphicsItem::setCenter(double centerX, double centerY, ContentWindowInterface * source)
{
    ContentWindowInterface::setCenter(centerX, centerY, source);

    if(source != this)
    {
        // force a redraw to update window info label
        update();
    }
}

void ContentWindowGraphicsItem::setZoom(double zoom, ContentWindowInterface * source)
{
    ContentWindowInterface::setZoom(zoom, source);

    if(source != this)
    {
        // force a redraw to update window info label
        update();
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

        // force a redraw to update window info label
        update();
    }
}

void ContentWindowGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    // on Mac we've seen that mouse events can go to the wrong graphics item
    // this is due to the bug: https://bugreports.qt.nokia.com/browse/QTBUG-20493
    // here we ignore the event if it shouldn't have been sent to us, which ensures
    // it will go to the correct item...
    if(boundingRect().contains(event->pos()) == false)
    {
        event->ignore();
        return;
    }

    // button dimensions
    float buttonWidth, buttonHeight;
    getButtonDimensions(buttonWidth, buttonHeight);

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

    // move to the front of the GUI display
    moveToFront();

    QGraphicsItem::mousePressEvent(event);
}

void ContentWindowGraphicsItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event)
{
    // on Mac we've seen that mouse events can go to the wrong graphics item
    // this is due to the bug: https://bugreports.qt.nokia.com/browse/QTBUG-20493
    // here we ignore the event if it shouldn't have been sent to us, which ensures
    // it will go to the correct item...
    if(boundingRect().contains(event->pos()) == false)
    {
        event->ignore();
        return;
    }

    bool selected = !selected_;

    setSelected(selected);

    QGraphicsItem::mouseDoubleClickEvent(event);
}

void ContentWindowGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    resizing_ = false;

    QGraphicsItem::mouseReleaseEvent(event);
}

void ContentWindowGraphicsItem::wheelEvent(QGraphicsSceneWheelEvent * event)
{
    // on Mac we've seen that mouse events can go to the wrong graphics item
    // this is due to the bug: https://bugreports.qt.nokia.com/browse/QTBUG-20493
    // here we ignore the event if it shouldn't have been sent to us, which ensures
    // it will go to the correct item...
    if(boundingRect().contains(event->pos()) == false)
    {
        event->ignore();
        return;
    }

    // handle wheel movements differently depending on selected mode of item
    if(selected_ == false)
    {
        // scale size based on wheel delta
        // typical delta value is 120, so scale based on that
        double factor = 1. + (double)event->delta() / (10. * 120.);

        scaleSize(factor);
    }
    else
    {
        // change zoom based on wheel delta
        // deltas are counted in 1/8 degrees. so, scale based on 180 degrees => delta = 180*8 = 1440
        double zoomDelta = (double)event->delta() / 1440.;
        double zoom = zoom_ * (1. + zoomDelta);

        setZoom(zoom);
    }
}
