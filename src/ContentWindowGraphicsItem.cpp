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

namespace
{
    // how often (in ms) a drag broadcasts its position to the rest of the
    // cluster while the mouse is still moving - see mouseMoveEvent. 0 (the
    // default) means don't sync until mouse-up at all: no MPI traffic
    // mid-drag, other hosts just see the window jump to its final spot
    // when you let go. Override via DISPLAYCLUSTER_DRAG_SYNC_MS for
    // periodic mid-drag updates instead, on setups that can afford the
    // extra cluster round-trips and want other hosts to track the drag
    // more closely.
    int
    dragSyncIntervalMs()
    {
        static const int interval = (getenv("DISPLAYCLUSTER_DRAG_SYNC_MS") == NULL) ? 0 : atoi(getenv("DISPLAYCLUSTER_DRAG_SYNC_MS"));
        return interval;
    }
}

ContentWindowGraphicsItem::ContentWindowGraphicsItem(boost::shared_ptr<ContentWindowManager> contentWindowManager) : ContentWindowInterface(contentWindowManager)
{
    // defaults
    resizing_ = false;
    positionDirty_ = false;
    sizeDirty_ = false;
    zoomDirty_ = false;
    centerDirty_ = false;
    dragSyncTimer_.start();

    // graphics items are movable
    setFlag(QGraphicsItem::ItemIsMovable, true);

    // default fill color / opacity; reflects hidden state if set before construction
    setBrush(QBrush(hidden_ ? QColor(0, 0, 180, 128) : QColor(0, 0, 0, 128)));

    // border based on if we're selected or not
    // use the -1 argument to force an update but not emit signals
    setSelected(selected_, (ContentWindowInterface *)-1);

    // current coordinates
    setRect(x_, y_, w_, h_);

    // new items at the front
    // we assume that interface items will be constructed in depth order so this produces the correct result...
    setZToFront();
}

bool
ContentWindowGraphicsItem::dragSyncReady()
{
    int syncIntervalMs = dragSyncIntervalMs();

    if(syncIntervalMs > 0 && dragSyncTimer_.elapsed() >= syncIntervalMs)
    {
        dragSyncTimer_.restart();
        return true;
    }

    return false;
}

void ContentWindowGraphicsItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    QGraphicsRectItem::paint(painter, option, widget);

    boost::shared_ptr<ContentWindowManager> contentWindowManager = getContentWindowManager();

    if(contentWindowManager != NULL)
    {
        // save/restore around this item's paint work: the scale() calls
        // below permanently mutate the painter's transform if not undone,
        // corrupting whatever gets drawn next in the same paint pass
        // (tile-boundary rects, other content items, etc.)
        painter->save();

        // default pen
        // cosmetic + width 0: fixed device-pixel width, not scaled by the
        // item's scene-to-pixel transform (the scene is a 1x1 unit square,
        // so a non-cosmetic width-1 pen renders hundreds of pixels thick)
        QPen pen;
        pen.setCosmetic(true);
        pen.setWidth(0);

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

        painter->restore();
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
        // cosmetic + width 0: fixed device-pixel width, not scaled by the
        // item's scene-to-pixel transform. Width must be 0 here, not just
        // cosmetic - QGraphicsRectItem::boundingRect() pads itself using
        // pen().widthF() in local scene units regardless of the cosmetic
        // flag (cosmetic only affects rendering), so a width-1 pen in this
        // 1x1-unit scene inflates the item's hit-test region by 0.5 units
        // in every direction, large enough to swallow every other item's
        // clicks
        QPen p = pen();
        p.setCosmetic(true);
        p.setWidth(0);

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

void ContentWindowGraphicsItem::setHidden(bool hidden, ContentWindowInterface * source)
{
    ContentWindowInterface::setHidden(hidden, source);

    if(source != this)
    {
        // change fill color to indicate hidden state:
        // hidden  -> dark blue tint  (semi-transparent)
        // visible -> normal black fill (semi-transparent)
        if(hidden_)
        {
            setBrush(QBrush(QColor(0, 0, 180, 128)));
        }
        else
        {
            setBrush(QBrush(QColor(0, 0, 0, 128)));
        }

        // force a redraw
        update();
    }
}

void ContentWindowGraphicsItem::setZToFront()
{
    zCounter_ = zCounter_ + 1;
    setZValue(zCounter_);
}

// Every branch below follows the same shape: update local state/visuals
// on every mouse-move event via the (ContentWindowInterface *)-1 sentinel
// (the same "force an update but don't emit signals" trick already used
// in the constructor for setSelected()), so this host's own view stays
// fully responsive regardless of sync rate. The *real* setter (default
// source = NULL) is only called when dragSyncReady() allows it, since it
// emits a signal that's wired straight to DisplayGroupManager::
// sendDisplayGroup() - a blocking MPI_Bcast + MPI_Barrier across every
// render host in the cluster. Calling that on every mouse-move event
// (which fires far more often than a cluster round-trip takes) makes
// dragging/resizing/zooming feel like it hangs, since the GUI thread
// blocks on the whole cluster per pixel of movement. See
// dragSyncIntervalMs() above for the default (off) and how to override
// it; mouseReleaseEvent flushes any leftover dirty state so the final
// values are never dropped regardless of interval.
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

                setSize(w, h, (ContentWindowInterface *)-1);

                if(dragSyncReady())
                {
                    setSize(w_, h_);
                    sizeDirty_ = false;
                }
                else
                {
                    sizeDirty_ = true;
                }
            }
            else
            {
                QPointF delta = event->pos() - event->lastPos();

                double x = x_ + delta.x();
                double y = y_ + delta.y();

                setPosition(x, y, (ContentWindowInterface *)-1);

                if(dragSyncReady())
                {
                    setPosition(x_, y_);
                    positionDirty_ = false;
                }
                else
                {
                    positionDirty_ = true;
                }
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
            double zoomDelta = delta.y();

            double zoom = zoom_ * (1. - zoomDelta);

            setZoom(zoom, (ContentWindowInterface *)-1);

            if(dragSyncReady())
            {
                setZoom(zoom_);
                zoomDirty_ = false;
            }
            else
            {
                zoomDirty_ = true;
            }
        }
        else if(event->buttons().testFlag(Qt::LeftButton) == true)
        {
            // pan (move center coordinates)
            double centerX = centerX_ + 2.*delta.x() / zoom_;
            double centerY = centerY_ + 2.*delta.y() / zoom_;

            setCenter(centerX, centerY, (ContentWindowInterface *)-1);

            if(dragSyncReady())
            {
                setCenter(centerX_, centerY_);
                centerDirty_ = false;
            }
            else
            {
                centerDirty_ = true;
            }
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

    // flush any drag update that was throttled away mid-drag (see
    // mouseMoveEvent/dragSyncIntervalMs()) so the final state always
    // reaches the rest of the cluster, even if it landed inside the last
    // throttle interval
    if(positionDirty_)
    {
        setPosition(x_, y_);
        positionDirty_ = false;
    }
    if(sizeDirty_)
    {
        setSize(w_, h_);
        sizeDirty_ = false;
    }
    if(zoomDirty_)
    {
        setZoom(zoom_);
        zoomDirty_ = false;
    }
    if(centerDirty_)
    {
        setCenter(centerX_, centerY_);
        centerDirty_ = false;
    }

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
