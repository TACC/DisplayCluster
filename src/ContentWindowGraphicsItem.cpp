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
#include "Gestures.h"
#include "Dock.h"
#include "Pictureflow.h"

qreal ContentWindowGraphicsItem::zCounter_ = 0;

ContentWindowGraphicsItem::ContentWindowGraphicsItem(boost::shared_ptr<ContentWindowManager> contentWindowManager) : ContentWindowInterface(contentWindowManager)
{
    // defaults
    resizing_ = false;
    moving_ = false;

    // graphics items are movable
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);

    // border based on if we're selected or not
    // use the -1 argument to force an update but not emit signals
    setWindowState(windowState_, (ContentWindowInterface *)-1);

    // new items at the front
    // we assume that interface items will be constructed in depth order so this produces the correct result...
    setZToFront();

    grabGesture( Qt::PinchGesture );
    grabGesture( PanGestureRecognizer::type( ));
    grabGesture( DoubleTapGestureRecognizer::type( ));
    //grabGesture( Qt::SwipeGesture );
    grabGesture( Qt::TapAndHoldGesture );
    grabGesture( Qt::TapGesture );
}

QRectF ContentWindowGraphicsItem::boundingRect() const
{
    return QRectF(x_, y_, w_, h_);
}

void ContentWindowGraphicsItem::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    boost::shared_ptr<ContentWindowManager> contentWindowManager = getContentWindowManager();

    if(!contentWindowManager)
        return;

    QPen pen;
    if(windowState_ == UNSELECTED)
        pen.setColor(QColor(0,0,0));
    else if(windowState_ == SELECTED)
        pen.setColor(QColor(255,0,0));
    else if(windowState_ == INTERACTION)
        pen.setColor(QColor(0,255,0));
    else
        pen.setColor(QColor(0,0,0));

    // draw & fill rectangle
    painter->setPen(pen);
    painter->setBrush( QBrush(QColor(0, 0, 0, 128)));
    painter->drawRect(boundingRect());

    // button dimensions
    float buttonWidth, buttonHeight;
    getButtonDimensions(buttonWidth, buttonHeight);

    const qreal& x = x_;
    const qreal& y = y_;
    const qreal& w = w_;
    const qreal& h = h_;

    // draw close button
    QRectF closeRect(x + w - buttonWidth, y, buttonWidth, buttonHeight);
    pen.setColor(QColor(255,0,0));
    painter->setPen(pen);
    painter->drawRect(closeRect);
    painter->drawLine(QPointF(x + w - buttonWidth, y), QPointF(x + w, y + buttonHeight));
    painter->drawLine(QPointF(x + w, y), QPointF(x + w - buttonWidth, y + buttonHeight));

    // resize indicator
    QRectF resizeRect(x + w - buttonWidth, y + h - buttonHeight, buttonWidth, buttonHeight);
    pen.setColor(QColor(128,128,128));
    painter->setPen(pen);
    painter->drawRect(resizeRect);
    painter->drawLine(QPointF(x + w, y + h - buttonHeight), QPointF(x + w - buttonWidth, y + h));

    // fullscreen button
    QRectF fullscreenRect(x, y + h - buttonHeight, buttonWidth, buttonHeight);
    painter->setPen(pen);
    painter->drawRect(fullscreenRect);

    if( contentWindowManager->getContent()->getType() == CONTENT_TYPE_MOVIE &&
        g_displayGroupManager->getOptions()->getShowMovieControls( ))
    {
        // play/pause
        QRectF playPauseRect(x + w/2 - buttonWidth, y + h - buttonHeight,
                              buttonWidth, buttonHeight);
        pen.setColor(QColor(contentWindowManager->getControlState() & STATE_PAUSED ? 128 :200,0,0));
        painter->setPen(pen);
        painter->fillRect(playPauseRect, pen.color());

        // loop
        QRectF loopRect(x + w/2, y + h - buttonHeight,
                        buttonWidth, buttonHeight);
        pen.setColor(QColor(0,contentWindowManager->getControlState() & STATE_LOOP ? 200 :128,0));
        painter->setPen(pen);
        painter->fillRect(loopRect, pen.color());
    }

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

    QRectF textBoundingRect = QRectF(x / horizontalTextScale, y / verticalTextScale, w / horizontalTextScale, h / verticalTextScale);

    // get the label and render it
    QString label(contentWindowManager->getContent()->getURI().c_str());
    QString labelSection = label.section("/", -1, -1).prepend(" ");
    painter->drawText(textBoundingRect, Qt::AlignLeft | Qt::AlignTop, labelSection);

    // draw window info at smaller scale
    verticalTextScale *= 0.5;
    horizontalTextScale *= 0.5;

    painter->scale(0.5, 0.5);

    textBoundingRect = QRectF((x+buttonWidth) / horizontalTextScale, y / verticalTextScale, (w-buttonWidth) / horizontalTextScale, h / verticalTextScale);

    QString coordinatesLabel = QString(" (") + QString::number(x_, 'f', 2) + QString(" ,") + QString::number(y_, 'f', 2) + QString(", ") + QString::number(w_, 'f', 2) + QString(", ") + QString::number(h_, 'f', 2) + QString(")\n");
    QString zoomCenterLabel = QString(" zoom = ") + QString::number(zoom_, 'f', 2) + QString(" @ (") + QString::number(centerX_, 'f', 2) + QString(", ") + QString::number(centerY_, 'f', 2) + QString(")");
    QString interactionLabel = QString(" x: ") +
            QString::number(interactionState_.mouseX, 'f', 2) +
            QString(" y: ") + QString::number(interactionState_.mouseY, 'f', 2) +
            QString(" mouseLeft: ") + QString::number((int) interactionState_.mouseLeft, 'b', 1) +
            QString(" mouseMiddle: ") + QString::number((int) interactionState_.mouseMiddle, 'b', 1) +
            QString(" mouseRight: ") + QString::number((int) interactionState_.mouseRight, 'b', 1);

    QString windowInfoLabel = coordinatesLabel + zoomCenterLabel + interactionLabel;
    painter->drawText(textBoundingRect, Qt::AlignLeft | Qt::AlignBottom, windowInfoLabel);
}

void ContentWindowGraphicsItem::adjustSize( const SizeState state,
                                            ContentWindowInterface * source )
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::adjustSize( state, source );
}

void ContentWindowGraphicsItem::setCoordinates(double x, double y, double w, double h, ContentWindowInterface * source)
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::setCoordinates(x, y, w, h, source);
}

void ContentWindowGraphicsItem::setPosition(double x, double y, ContentWindowInterface * source)
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::setPosition(x, y, source);
}

void ContentWindowGraphicsItem::setSize(double w, double h, ContentWindowInterface * source)
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::setSize(w, h, source);
}

void ContentWindowGraphicsItem::setCenter(double centerX, double centerY, ContentWindowInterface * source)
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::setCenter(centerX, centerY, source);
}

void ContentWindowGraphicsItem::setZoom(double zoom, ContentWindowInterface * source)
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::setZoom(zoom, source);
}

void ContentWindowGraphicsItem::setWindowState(ContentWindowInterface::WindowState windowState, ContentWindowInterface * source)
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::setWindowState(windowState, source);
}

void ContentWindowGraphicsItem::setInteractionState(InteractionState interactionState, ContentWindowInterface * source)
{
    if(source != this)
        prepareGeometryChange();

    ContentWindowInterface::setInteractionState(interactionState, source);
}

void ContentWindowGraphicsItem::setZToFront()
{
    zCounter_ = zCounter_ + 1;
    setZValue(zCounter_);
}

bool ContentWindowGraphicsItem::sceneEvent( QEvent* event )
{
    switch( event->type( ))
    {
    case QEvent::Gesture:
        gestureEvent( static_cast< QGestureEvent* >( event ));
        return true;
    default:
        return QGraphicsObject::sceneEvent( event );
    }
}

void ContentWindowGraphicsItem::gestureEvent( QGestureEvent* event )
{
    if( !getContentWindowManager( ))
        return;

    moveToFront();

    if( QGesture *gesture = event->gesture( Qt::SwipeGesture ))
    {
        event->accept( Qt::SwipeGesture );
        swipe( static_cast< QSwipeGesture* >( gesture ));
    }
    else if(QGesture* gesture = event->gesture( PanGestureRecognizer::type( )))
    {
        event->accept( PanGestureRecognizer::type( ));
        pan( static_cast< PanGesture* >( gesture ));
    }
    else if(QGesture* gesture = event->gesture( Qt::PinchGesture ))
    {
        event->accept( Qt::PinchGesture );
        pinch( static_cast< QPinchGesture* >( gesture ));
    }
    else if( QGesture* gesture = event->gesture( DoubleTapGestureRecognizer::type( )))
    {
        event->accept( DoubleTapGestureRecognizer::type( ));
        doubleTap( static_cast< DoubleTapGesture* >( gesture ));
    }
    else if( QGesture* gesture = event->gesture( Qt::TapGesture ))
    {
        event->accept( Qt::TapGesture );
        tap( static_cast< QTapGesture* >( gesture ));
    }
    else if( QGesture* gesture = event->gesture( Qt::TapAndHoldGesture ))
    {
        event->accept( Qt::TapAndHoldGesture );
        tapAndHold( static_cast< QTapAndHoldGesture* >( gesture ));
    }
}

void ContentWindowGraphicsItem::swipe( QSwipeGesture* gesture )
{
    std::cout << "SWIPE " << gesture->state() << std::endl;
}

void ContentWindowGraphicsItem::pan( PanGesture* gesture )
{
    const QPointF& delta = gesture->delta();
    const double dx = delta.x() / g_configuration->getTotalWidth();
    const double dy = delta.y() / g_configuration->getTotalHeight();

    if( getContentWindowManager()->getContent()->isDock( ))
    {
        const int offs = delta.x()/4;
        g_dock->getFlow()->showSlide( g_dock->getFlow()->centerIndex() + offs );
        return;
    }

    if( windowState_ == SELECTED )
    {
        const double centerX = centerX_ - 2.*dx / zoom_;
        const double centerY = centerY_ - 2.*dy / zoom_;
        setCenter(centerX, centerY);
        return;
    }

    if( windowState_ == INTERACTION )
    {
        InteractionState interactionState = interactionState_;

        interactionState.mouseX = gesture->position().x();
        interactionState.mouseY = gesture->position().y();
        interactionState.mouseLeft = true;
        switch( gesture->state( ))
        {
        case Qt::GestureStarted:
            interactionState.type = InteractionState::EVT_PRESS;
            break;
        case Qt::GestureUpdated:
            interactionState.type = InteractionState::EVT_MOVE;
            break;
        case Qt::GestureFinished:
            interactionState.type = InteractionState::EVT_RELEASE;
            break;
        case Qt::NoGesture:
        case Qt::GestureCanceled:
        default:
            break;
        }

        setInteractionState(interactionState);
        return;
    }

    if( gesture->state() == Qt::GestureStarted )
        getContentWindowManager()->getContent()->blockAdvance( true );

    const double x = x_ + dx;
    const double y = y_ + dy;
    setPosition( x, y );

    if( gesture->state() == Qt::GestureCanceled ||
        gesture->state() == Qt::GestureFinished )
    {
        getContentWindowManager()->getContent()->blockAdvance( false );
    }
}

void ContentWindowGraphicsItem::pinch( QPinchGesture* gesture )
{
    if( getContentWindowManager()->getContent()->isDock( ))
        return;

    const qreal factor = (gesture->scaleFactor() - 1.) * 0.2f + 1.f;
    if( std::isnan( factor ) || std::isinf( factor ))
        return;

    if( windowState_ == SELECTED )
    {
        setZoom( getZoom() * factor );
        return;
    }

    if( windowState_ == INTERACTION )
    {
        InteractionState interactionState = interactionState_;
        interactionState.dy = factor - 1.f;
        interactionState.type = InteractionState::EVT_WHEEL;

        setInteractionState(interactionState);
        return;
    }

    if( gesture->state() == Qt::GestureStarted )
        getContentWindowManager()->getContent()->blockAdvance( true );

    scaleSize( factor );

    if( gesture->state() == Qt::GestureCanceled ||
        gesture->state() == Qt::GestureFinished )
    {
        getContentWindowManager()->getContent()->blockAdvance( false );
    }
}

void ContentWindowGraphicsItem::tap( QTapGesture* gesture )
{
//    if( windowState_ == INTERACTION && gesture->state() != Qt::GestureCanceled )
//    {
//        InteractionState interactionState = interactionState_;

//        interactionState.mouseX = gesture->position().x() / w_;
//        interactionState.mouseY = gesture->position().y() / h_;
//        interactionState.mouseLeft = true;
//        interactionState.type = gesture->state() == Qt::GestureStarted ?
//                    InteractionState::EVT_PRESS : InteractionState::EVT_RELEASE;

//        setInteractionState(interactionState);
//        return;
//    }

    if( gesture->state() != Qt::GestureFinished )
        return;

    if( !getContentWindowManager()->getContent()->isDock( ))
        return;

    const int xPos = gesture->position().x() - (x_*g_configuration->getTotalWidth());
    const int mid = (w_*g_configuration->getTotalWidth())/2;
    const int slideMid = g_dock->getFlow()->slideSize().width()/2;

    if( xPos > mid-slideMid && xPos < mid+slideMid )
    {
        g_dock->onItem();
        return;
    }

    if( xPos > mid )
      g_dock->getFlow()->showNext();
    else
      g_dock->getFlow()->showPrevious();
}

void ContentWindowGraphicsItem::doubleTap( DoubleTapGesture* gesture )
{
    if( getContentWindowManager()->getContent()->isDock( ))
        return;

    if( windowState_ != UNSELECTED )
        return;

    if( gesture->state() == Qt::GestureFinished )
        adjustSize( getSizeState() == SIZE_FULLSCREEN ? SIZE_1TO1 :
                                                        SIZE_FULLSCREEN );
}

void ContentWindowGraphicsItem::tapAndHold( QTapAndHoldGesture* gesture )
{
    if( getContentWindowManager()->getContent()->isDock( ))
        return;

    if( gesture->state() != Qt::GestureFinished )
        return;

    // move to next state
    switch( windowState_ )
    {
    case UNSELECTED:
        windowState_ = SELECTED;
        break;
    case SELECTED:
        windowState_ = INTERACTION;
        break;
    case INTERACTION:
        windowState_ = UNSELECTED;
        break;
    }

    setWindowState( windowState_ );
}

void ContentWindowGraphicsItem::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    // handle mouse movements differently depending on selected mode of item
    if(windowState_ == UNSELECTED)
    {
        if(event->buttons().testFlag(Qt::LeftButton) == true)
        {
            if(resizing_ == true)
            {
                QRectF r = boundingRect();
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
    else if(windowState_ == SELECTED)
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
    else if(windowState_ == INTERACTION)
    {
        QRectF r = boundingRect();
        QPointF eventPos = event->pos();

        InteractionState interactionState = interactionState_;

        interactionState.mouseX = (eventPos.x() - r.x()) / r.width();
        interactionState.mouseY = (eventPos.y() - r.y()) / r.height();

        interactionState.mouseLeft = event->buttons().testFlag(Qt::LeftButton);
        interactionState.mouseMiddle = event->buttons().testFlag(Qt::MidButton);
        interactionState.mouseRight = event->buttons().testFlag(Qt::RightButton);
        interactionState.type = InteractionState::EVT_MOVE;

        setInteractionState(interactionState);

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

    if(windowState_ == INTERACTION)
    {
        QRectF r = boundingRect();
        QPointF eventPos = event->pos();

        InteractionState interactionState = interactionState_;

        interactionState.mouseX = (eventPos.x() - r.x()) / r.width();
        interactionState.mouseY = (eventPos.y() - r.y()) / r.height();

        interactionState.mouseLeft = event->buttons().testFlag(Qt::LeftButton);
        interactionState.mouseMiddle = event->buttons().testFlag(Qt::MidButton);
        interactionState.mouseRight = event->buttons().testFlag(Qt::RightButton);
        interactionState.type = InteractionState::EVT_PRESS;

        setInteractionState(interactionState);
    }

    // button dimensions
    float buttonWidth, buttonHeight;
    getButtonDimensions(buttonWidth, buttonHeight);

    // item rectangle and event position
    QRectF r = boundingRect();
    QPointF eventPos = event->pos();

    // check to see if user clicked on the close button
    if(fabs((r.x()+r.width()) - eventPos.x()) <= buttonWidth &&
       fabs(r.y() - eventPos.y()) <= buttonHeight)
    {
        close();

        return;
    }

    // move to the front of the GUI display
    moveToFront();

    if( selected( ))
        return;

    boost::shared_ptr<ContentWindowManager> window = getContentWindowManager();

    window->getContent()->blockAdvance( true );

    // check to see if user clicked on the resize button
    if(fabs((r.x()+r.width()) - eventPos.x()) <= buttonWidth &&
       fabs((r.y()+r.height()) - eventPos.y()) <= buttonHeight)
    {
        resizing_ = true;
    }
    // check to see if user clicked on the fullscreen button
    else if(fabs(r.x() - eventPos.x()) <= buttonWidth &&
            fabs((r.y()+r.height()) - eventPos.y()) <= buttonHeight)
    {
        adjustSize( getSizeState() == SIZE_FULLSCREEN ? SIZE_1TO1 : SIZE_FULLSCREEN );
    }
    else if(fabs(((r.x()+r.width())/2) - eventPos.x() - buttonWidth) <= buttonWidth &&
            fabs((r.y()+r.height()) - eventPos.y()) <= buttonHeight &&
            g_displayGroupManager->getOptions()->getShowMovieControls( ))
    {
        window->setControlState( ControlState(window->getControlState() ^ STATE_PAUSED) );
    }
    else if(fabs(((r.x()+r.width())/2) - eventPos.x()) <= buttonWidth &&
            fabs((r.y()+r.height()) - eventPos.y()) <= buttonHeight &&
            g_displayGroupManager->getOptions()->getShowMovieControls( ))
    {
        window->setControlState( ControlState(window->getControlState() ^ STATE_LOOP) );
    }
    else
        moving_ = true;

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

    // move to next state
    switch(windowState_)
    {
        case UNSELECTED:
            windowState_ = SELECTED;
            break;
        case SELECTED:
            windowState_ = INTERACTION;
            break;
        case INTERACTION:
            windowState_ = UNSELECTED;
            break;
    }

    setWindowState(windowState_);

    QGraphicsItem::mouseDoubleClickEvent(event);
}

void ContentWindowGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    resizing_ = false;
    moving_ = false;
    if( getContentWindowManager( ))
        getContentWindowManager()->getContent()->blockAdvance( false );

    if(windowState_ == INTERACTION)
    {
        QRectF r = boundingRect();
        QPointF eventPos = event->pos();

        InteractionState interactionState = interactionState_;

        interactionState.mouseX = (eventPos.x() - r.x()) / r.width();
        interactionState.mouseY = (eventPos.y() - r.y()) / r.height();

        interactionState.mouseLeft = event->buttons().testFlag(Qt::LeftButton);
        interactionState.mouseMiddle = event->buttons().testFlag(Qt::MidButton);
        interactionState.mouseRight = event->buttons().testFlag(Qt::RightButton);
        interactionState.type = InteractionState::EVT_RELEASE;

        setInteractionState(interactionState);
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

    // handle wheel movements differently depending on state of item window
    if(windowState_ == UNSELECTED)
    {
        // scale size based on wheel delta
        // typical delta value is 120, so scale based on that
        double factor = 1. + (double)event->delta() / (10. * 120.);

        scaleSize(factor);
    }
    else if(windowState_ == SELECTED)
    {
        // change zoom based on wheel delta
        // deltas are counted in 1/8 degrees. so, scale based on 180 degrees => delta = 180*8 = 1440
        double zoomDelta = (double)event->delta() / 1440.;
        double zoom = zoom_ * (1. + zoomDelta);

        setZoom(zoom);
    }
    else if(windowState_ == INTERACTION)
    {
        QRectF r = boundingRect();
        QPointF eventPos = event->pos();

        InteractionState interactionState = interactionState_;

        interactionState.mouseX = (eventPos.x() - r.x()) / r.width();
        interactionState.mouseY = (eventPos.y() - r.y()) / r.height();

        interactionState.mouseLeft = event->buttons().testFlag(Qt::LeftButton);
        interactionState.mouseMiddle = event->buttons().testFlag(Qt::MidButton);
        interactionState.mouseRight = event->buttons().testFlag(Qt::RightButton);
        interactionState.type = InteractionState::EVT_WHEEL;
        interactionState.dy = (double)event->delta() / 1440.;

        setInteractionState(interactionState);
    }
}

void ContentWindowGraphicsItem::keyPressEvent(QKeyEvent *event)
{
    if(windowState_ != INTERACTION)
        return;

    InteractionState interactionState = interactionState_;
    interactionState.type = InteractionState::EVT_KEY_PRESS;
    interactionState.key = event->key();

    setInteractionState(interactionState);
}

void ContentWindowGraphicsItem::keyReleaseEvent(QKeyEvent *event)
{
    if(windowState_ != INTERACTION)
        return;

    InteractionState interactionState = interactionState_;
    interactionState.type = InteractionState::EVT_KEY_RELEASE;
    interactionState.key = event->key();

    setInteractionState(interactionState);
}
