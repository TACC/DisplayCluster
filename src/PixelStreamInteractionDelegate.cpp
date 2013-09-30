/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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

#include "PixelStreamInteractionDelegate.h"
#include "ContentWindowManager.h"
#include "main.h"

PixelStreamInteractionDelegate::PixelStreamInteractionDelegate(ContentWindowManager* cwm)
    : ContentInteractionDelegate(cwm)
{
}

void PixelStreamInteractionDelegate::tap(QTapGesture *gesture)
{
    if ( gesture->state() == Qt::GestureFinished )
    {
        InteractionState interactionState = getGestureInteractionState(gesture);

        interactionState.mouseLeft = true;
        interactionState.type = InteractionState::EVT_CLICK;

        contentWindowManager_->setInteractionState(interactionState);
    }
}

void PixelStreamInteractionDelegate::doubleTap(DoubleTapGesture *gesture)
{
    InteractionState interactionState = getGestureInteractionState(gesture);

    interactionState.mouseLeft = true;
    interactionState.type = InteractionState::EVT_DOUBLECLICK;

    contentWindowManager_->setInteractionState(interactionState);
}

void PixelStreamInteractionDelegate::pan(PanGesture *gesture)
{
    InteractionState interactionState = getGestureInteractionState(gesture);

    interactionState.mouseLeft = true;

    switch( gesture->state( ) )
    {
    case Qt::GestureStarted:
        interactionState.type = InteractionState::EVT_PRESS;
        break;
    case Qt::GestureUpdated:
        interactionState.type = InteractionState::EVT_MOVE;
        setPanGestureNormalizedDelta(gesture, interactionState);
        break;
    case Qt::GestureFinished:
        interactionState.type = InteractionState::EVT_RELEASE;
        break;
    case Qt::NoGesture:
    case Qt::GestureCanceled:
    default:
        interactionState.type = InteractionState::EVT_NONE;
        break;
    }

    contentWindowManager_->setInteractionState(interactionState);
}

void PixelStreamInteractionDelegate::swipe(QSwipeGesture *gesture)
{
    InteractionState interactionState;

    if (gesture->horizontalDirection() == QSwipeGesture::Left)
    {
        interactionState.type = InteractionState::EVT_SWIPE_LEFT;
    }
    else if (gesture->horizontalDirection() == QSwipeGesture::Right)
    {
        interactionState.type = InteractionState::EVT_SWIPE_LEFT;
    }
    else if (gesture->verticalDirection() == QSwipeGesture::Up)
    {
        interactionState.type = InteractionState::EVT_SWIPE_UP;
    }
    else if (gesture->verticalDirection() == QSwipeGesture::Down)
    {
        interactionState.type = InteractionState::EVT_SWIPE_DOWN;
    }

    if (interactionState.type != InteractionState::EVT_NONE)
    {
        contentWindowManager_->setInteractionState(interactionState);
    }
}

void PixelStreamInteractionDelegate::pinch(QPinchGesture *gesture)
{
    const qreal factor = (gesture->scaleFactor() - 1.) * 0.2f + 1.f;
    if( std::isnan( factor ) || std::isinf( factor ))
        return;

    InteractionState interactionState = getGestureInteractionState(gesture);
    interactionState.dy = factor - 1.f;
    interactionState.type = InteractionState::EVT_WHEEL;

    contentWindowManager_->setInteractionState(interactionState);
}


void PixelStreamInteractionDelegate::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    InteractionState interactionState = getMouseInteractionState(event);
    interactionState.type = InteractionState::EVT_MOVE;

    setMouseMoveNormalizedDelta(event, interactionState);

    contentWindowManager_->setInteractionState(interactionState);
}

void PixelStreamInteractionDelegate::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    InteractionState interactionState = getMouseInteractionState(event);
    interactionState.type = InteractionState::EVT_PRESS;

    mousePressPos_ = event->pos();

    contentWindowManager_->setInteractionState(interactionState);
}

void PixelStreamInteractionDelegate::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    InteractionState interactionState = getMouseInteractionState(event);
    interactionState.type = InteractionState::EVT_DOUBLECLICK;

    contentWindowManager_->setInteractionState(interactionState);
}

void PixelStreamInteractionDelegate::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    InteractionState interactionState = getMouseInteractionState(event);
    interactionState.type = InteractionState::EVT_RELEASE;

    contentWindowManager_->setInteractionState(interactionState);

    // Also generate a click event if releasing the button in place
    if ( fabs(mousePressPos_.x() - event->pos().x()) < std::numeric_limits< float >::epsilon() &&
         fabs(mousePressPos_.y() - event->pos().y()) < std::numeric_limits< float >::epsilon() )
    {
        interactionState.type = InteractionState::EVT_CLICK;
        contentWindowManager_->setInteractionState(interactionState);
    }
}

void PixelStreamInteractionDelegate::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    InteractionState interactionState = getMouseInteractionState(event);

    interactionState.type = InteractionState::EVT_WHEEL;

    if (event->orientation() == Qt::Vertical)
    {
        interactionState.dx = 0.;
        interactionState.dy = (double)event->delta() / 1440.;
    }
    else
    {
        interactionState.dx = (double)event->delta() / 1440.;
        interactionState.dy = 0.;
    }

    contentWindowManager_->setInteractionState(interactionState);
}

void PixelStreamInteractionDelegate::keyPressEvent(QKeyEvent *event)
{
    InteractionState interactionState = contentWindowManager_->getInteractionState();
    interactionState.type = InteractionState::EVT_KEY_PRESS;
    interactionState.key = event->key();
    interactionState.modifiers = event->modifiers();
    interactionState.text = event->text().toStdString();

    contentWindowManager_->setInteractionState(interactionState);
}

void PixelStreamInteractionDelegate::keyReleaseEvent(QKeyEvent *event)
{
    InteractionState interactionState = contentWindowManager_->getInteractionState();
    interactionState.type = InteractionState::EVT_KEY_RELEASE;
    interactionState.key = event->key();
    interactionState.modifiers = event->modifiers();
    interactionState.text = event->text().toStdString();

    contentWindowManager_->setInteractionState(interactionState);
}

template <typename T>
InteractionState PixelStreamInteractionDelegate::getMouseInteractionState(const T *mouseEvent)
{
    // Bounding rectangle
    double x, y, w, h;
    contentWindowManager_->getCoordinates(x, y, w, h);

    QPointF eventPos = mouseEvent->pos();

    InteractionState interactionState;

    // Normalize mouse coordinates
    interactionState.mouseX = (eventPos.x() - x) / w;
    interactionState.mouseY = (eventPos.y() - y) / h;

    interactionState.mouseLeft = mouseEvent->buttons().testFlag(Qt::LeftButton);
    interactionState.mouseMiddle = mouseEvent->buttons().testFlag(Qt::MidButton);
    interactionState.mouseRight = mouseEvent->buttons().testFlag(Qt::RightButton);

    return interactionState;
}

void PixelStreamInteractionDelegate::setMouseMoveNormalizedDelta(const QGraphicsSceneMouseEvent *event, InteractionState& interactionState)
{
    // Bounding rectangle
    double w, h;
    contentWindowManager_->getSize(w, h);

    interactionState.dx = (event->pos().x() - event->lastPos().x()) / w;
    interactionState.dy = (event->pos().y() - event->lastPos().y()) / h;
}


/** Returns an new InteractionState with normalized mouse coordinates */

template<typename T>
InteractionState PixelStreamInteractionDelegate::getGestureInteractionState(const T *gesture)
{
    // Bounding rectangle
    double x, y, w, h;
    contentWindowManager_->getCoordinates(x, y, w, h);

    InteractionState interactionState;
    interactionState.mouseX = (gesture->position().x() - x) / w;
    interactionState.mouseY = (gesture->position().y() - y) / h;
    return interactionState;
}

InteractionState PixelStreamInteractionDelegate::getGestureInteractionState(const QTapGesture *gesture)
{
    // Bounding rectangle
    double x, y, w, h;
    contentWindowManager_->getCoordinates(x, y, w, h);

    // Touchpad dimensions
    const double tWidth = g_configuration->getTotalWidth();
    const double tHeight = g_configuration->getTotalHeight();

    InteractionState interactionState;
    interactionState.mouseX = (gesture->position().x() / tWidth - x) / w;
    interactionState.mouseY = (gesture->position().y() / tHeight - y) / h;

    return interactionState;
}

InteractionState PixelStreamInteractionDelegate::getGestureInteractionState(const QPinchGesture *gesture)
{
    // Bounding rectangle
    double x, y, w, h;
    contentWindowManager_->getCoordinates(x, y, w, h);

    // Touchpad dimensions
    const double tWidth = g_configuration->getTotalWidth();
    const double tHeight = g_configuration->getTotalHeight();

    InteractionState interactionState;
    interactionState.mouseX = (gesture->centerPoint().x() / tWidth - x) / w;
    interactionState.mouseY = (gesture->centerPoint().y() / tHeight - y) / h;

    return interactionState;
}

void PixelStreamInteractionDelegate::setPanGestureNormalizedDelta(const PanGesture* gesture, InteractionState& interactionState)
{
    // Bounding rectangle
    double w, h;
    contentWindowManager_->getSize(w, h);

    // Touchpad dimensions
    const double tWidth = g_configuration->getTotalWidth();
    const double tHeight = g_configuration->getTotalHeight();

    interactionState.dx = (gesture->delta().x() / tWidth) / w;
    interactionState.dy = (gesture->delta().y() / tHeight) / h;
}

