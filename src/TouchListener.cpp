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

#include "TouchListener.h"
#include "main.h"
#include "DisplayGroupGraphicsViewProxy.h"
#include "DisplayGroupGraphicsView.h"

//#define TABLET_INTERACTION

TouchListener::TouchListener()
{
    graphicsViewProxy_ = new DisplayGroupGraphicsViewProxy(g_displayGroupManager);

    client_.addTuioListener(this);
    client_.connect();

#ifdef TABLET_INTERACTION
    cursorPos_ = QPointF( .5, .5 );
#endif
}

void TouchListener::addTuioObject(TUIO::TuioObject *tobj)
{

}

void TouchListener::updateTuioObject(TUIO::TuioObject *tobj)
{

}

void TouchListener::removeTuioObject(TUIO::TuioObject *tobj)
{

}

void TouchListener::addTuioCursor(TUIO::TuioCursor *tcur)
{
#ifdef TABLET_INTERACTION
    lastPoint_ = QPointF(tcur->getX(), tcur->getY());
#else
    QPointF point(tcur->getX(), tcur->getY());

    // figure out what kind of click this is
    int clickType = 0; // 0: no click, 1: single click, 2: double click

    QPointF delta1 = point - lastClickPoint1_;

    if(sqrtf(delta1.x()*delta1.x() + delta1.y()*delta1.y()) < DOUBLE_CLICK_DISTANCE && lastClickTime1_.elapsed() < DOUBLE_CLICK_TIME)
    {
        clickType = 1;

        QPointF delta2 = point - lastClickPoint2_;

        if(sqrtf(delta2.x()*delta2.x() + delta2.y()*delta2.y()) < DOUBLE_CLICK_DISTANCE && lastClickTime2_.elapsed() < DOUBLE_CLICK_TIME)
        {
            clickType = 2;
        }
    }

    // reset last click information
    lastClickTime2_ = lastClickTime1_;
    lastClickPoint2_ = lastClickPoint1_;

    lastClickTime1_.restart();
    lastClickPoint1_ = point;

    // create the mouse event
    QGraphicsSceneMouseEvent * event = NULL;

    if(clickType == 2)
    {
        event = new QGraphicsSceneMouseEvent(QEvent::GraphicsSceneMouseDoubleClick);
    }
    else if(clickType == 1)
    {
        event = new QGraphicsSceneMouseEvent(QEvent::GraphicsSceneMousePress);
    }

    if(event != NULL)
    {
        // set event parameters
        event->setScenePos(point);

        if(tcur->getCursorID() == 0)
        {
            event->setButton(Qt::LeftButton);
            event->setButtons(Qt::LeftButton);
        }
        else if(tcur->getCursorID() == 1)
        {
            event->setButton(Qt::RightButton);
            event->setButtons(Qt::RightButton);
        }

        // use alt keyboard modifier to indicate this is a touch event
        event->setModifiers(Qt::AltModifier);

        // post the event (thread-safe)
        QApplication::postEvent(graphicsViewProxy_->getGraphicsView()->scene(), event);
    }
#endif
}

void TouchListener::updateTuioCursor(TUIO::TuioCursor *tcur)
{
#ifdef TABLET_INTERACTION
    QGraphicsSceneMouseEvent* event = new QGraphicsSceneMouseEvent(QEvent::GraphicsSceneMouseMove);
    event->setLastScenePos( cursorPos_ );
    event->setButton( Qt::NoButton );

    const QPointF point( tcur->getX(), tcur->getY( ));
    cursorPos_ += point - lastPoint_;
    lastPoint_ = point;
    cursorPos_.setX( std::min( std::max( cursorPos_.x(), 0. ), 1. ));
    cursorPos_.setY( std::min( std::max( cursorPos_.y(), 0. ), 1. ));

    event->setScenePos( cursorPos_ );
    QApplication::postEvent(graphicsViewProxy_->getGraphicsView()->scene(), event);
#else
    // if more than one cursor is down, only accept cursor 1 for right movements
    if(client_.getTuioCursors().size() > 1 && tcur->getCursorID() != 1)
    {
        return;
    }

    QPointF point(tcur->getX(), tcur->getY());

    // create the mouse event
    QGraphicsSceneMouseEvent * event = new QGraphicsSceneMouseEvent(QEvent::GraphicsSceneMouseMove);

    // set event parameters
    event->setScenePos(point);
    event->setLastScenePos(lastPoint_);
    event->setButton(Qt::NoButton);

    // use alt keyboard modifier to indicate this is a touch event
    event->setModifiers(Qt::AltModifier);

    if(tcur->getCursorID() == 0)
    {
        event->setButtons(Qt::LeftButton);
    }
    else if(tcur->getCursorID() == 1)
    {
        event->setButtons(Qt::RightButton);
    }

    // post the event (thread-safe)
    QApplication::postEvent(graphicsViewProxy_->getGraphicsView()->scene(), event);

    // reset last point
    lastPoint_ = point;
#endif
}

void TouchListener::removeTuioCursor(TUIO::TuioCursor *tcur)
{
#ifndef TABLET_INTERACTION
    QPointF point(tcur->getX(), tcur->getY());

    // create the mouse event
    QGraphicsSceneMouseEvent * event = new QGraphicsSceneMouseEvent(QEvent::GraphicsSceneMouseRelease);

    // set event parameters
    event->setScenePos(point);

    // use alt keyboard modifier to indicate this is a touch event
    event->setModifiers(Qt::AltModifier);

    // note that we shouldn't call setButtons() here, since the release will only trigger an "ungrab" of the mouse
    // if there are no other buttons set
    if(tcur->getCursorID() == 0)
    {
        event->setButton(Qt::LeftButton);
    }
    else if(tcur->getCursorID() == 1)
    {
        event->setButton(Qt::RightButton);
    }

    // post the event (thread-safe)
    QApplication::postEvent(graphicsViewProxy_->getGraphicsView()->scene(), event);

    // reset last point
    lastPoint_ = point;
#endif
}

void TouchListener::refresh(TUIO::TuioTime frameTime)
{

}
