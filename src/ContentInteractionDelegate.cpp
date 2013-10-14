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

#include "ContentInteractionDelegate.h"

#include "ContentWindowManager.h"
#include "gestures/DoubleTapGesture.h"
#include "gestures/DoubleTapGestureRecognizer.h"
#include "gestures/PanGesture.h"
#include "gestures/PanGestureRecognizer.h"
#include "gestures/PinchGesture.h"
#include "gestures/PinchGestureRecognizer.h"

#include "globals.h"
#include "Configuration.h"


ContentInteractionDelegate::ContentInteractionDelegate(ContentWindowManager *cwm)
    : contentWindowManager_(cwm)
{
}


void ContentInteractionDelegate::gestureEvent( QGestureEvent* event )
{
    contentWindowManager_->moveToFront();

    if( QGesture* gesture = event->gesture( Qt::TapAndHoldGesture ))
    {
        event->accept( Qt::TapAndHoldGesture );
        tapAndHoldUnselected( static_cast< QTapAndHoldGesture* >( gesture ));
    }

    else if( QGesture* gesture = event->gesture( PanGestureRecognizer::type( )))
    {
        event->accept( PanGestureRecognizer::type( ));
        if( contentWindowManager_->selected() )
        {
            pan( static_cast< PanGesture* >( gesture ));
        }
        else
        {
            panUnselected(static_cast< PanGesture* >( gesture ));
        }
    }

    else if( QGesture* gesture = event->gesture( PinchGestureRecognizer::type( )))
    {
        event->accept( PinchGestureRecognizer::type( ));
        if( contentWindowManager_->selected() )
        {
            pinch( static_cast< PinchGesture* >( gesture ));
        }
        else
        {
            pinchUnselected( static_cast< PinchGesture* >( gesture ));
        }
    }

    else if( QGesture* gesture = event->gesture( DoubleTapGestureRecognizer::type( )))
    {
        event->accept( DoubleTapGestureRecognizer::type( ));
        if( contentWindowManager_->selected() )
        {
            doubleTap( static_cast< DoubleTapGesture* >( gesture ));
        }
        else
        {
            doubleTapUnselected(static_cast< DoubleTapGesture* >( gesture ));
        }
    }

    else if( QGesture* gesture = event->gesture( Qt::TapGesture ))
    {
        event->accept( Qt::TapGesture );
        if( contentWindowManager_->selected() )
        {
            tap( static_cast< QTapGesture* >( gesture ));
        }
    }

    else if( QGesture *gesture = event->gesture( Qt::SwipeGesture ))
    {
        event->accept( Qt::SwipeGesture );
        if( contentWindowManager_->selected() )
        {
            swipe( static_cast< QSwipeGesture* >( gesture ));
        }
    }
}


void ContentInteractionDelegate::tapAndHoldUnselected(QTapAndHoldGesture *gesture)
{
    if( gesture->state() == Qt::GestureFinished )
    {
        contentWindowManager_->toggleWindowState();
    }
}

void ContentInteractionDelegate::doubleTapUnselected(DoubleTapGesture *gesture)
{
    if( gesture->state() == Qt::GestureFinished )
    {
        contentWindowManager_->adjustSize( contentWindowManager_->getSizeState() == SIZE_FULLSCREEN ? SIZE_1TO1 : SIZE_FULLSCREEN );
    }
}

void ContentInteractionDelegate::panUnselected(PanGesture *gesture)
{
    const QPointF& delta = gesture->delta();
    const double dx = delta.x() / g_configuration->getTotalWidth();
    const double dy = delta.y() / g_configuration->getTotalHeight();

    if( gesture->state() == Qt::GestureStarted )
        contentWindowManager_->getContent()->blockAdvance( true );

    double x, y;
    contentWindowManager_->getPosition( x, y );
    contentWindowManager_->setPosition( x+dx, y+dy );

    if( gesture->state() == Qt::GestureCanceled ||
        gesture->state() == Qt::GestureFinished )
    {
        contentWindowManager_->getContent()->blockAdvance( false );
    }
}

void ContentInteractionDelegate::pinchUnselected(PinchGesture *gesture)
{
    const double factor = adaptZoomFactor(gesture->scaleFactor());
    if( factor == 0.0 )
        return;

    if( gesture->state() == Qt::GestureStarted )
        contentWindowManager_->getContent()->blockAdvance( true );

    contentWindowManager_->scaleSize( factor );

    if( gesture->state() == Qt::GestureCanceled ||
        gesture->state() == Qt::GestureFinished )
    {
        contentWindowManager_->getContent()->blockAdvance( false );
    }
}

double ContentInteractionDelegate::adaptZoomFactor(double pinchGestureScaleFactor)
{
    const double factor = (pinchGestureScaleFactor - 1.0) * 0.2 + 1.0;
    if( std::isnan( factor ) || std::isinf( factor ))
        return 0.0;
    else
        return factor;
}
