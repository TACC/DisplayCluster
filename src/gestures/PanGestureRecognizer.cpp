/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
/*                     Daniel Nachbaur <daniel.nachbaur@epfl.ch>     */
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

#include "PanGestureRecognizer.h"
#include "PanGesture.h"

#include <QtGui/QTouchEvent>
#include <QtGui/QWidget>


Qt::GestureType PanGestureRecognizer::_type = Qt::CustomGesture;

void PanGestureRecognizer::install()
{
    _type = QGestureRecognizer::registerRecognizer( new PanGestureRecognizer( 1 ));
}

void PanGestureRecognizer::uninstall()
{
    QGestureRecognizer::unregisterRecognizer( _type );
}

Qt::GestureType PanGestureRecognizer::type()
{
    return _type;
}

PanGestureRecognizer::PanGestureRecognizer( const int numPoints )
    : _nPoints( numPoints )
{
}

QGesture* PanGestureRecognizer::create( QObject* target )
{
    if( target && target->isWidgetType( ))
        static_cast< QWidget* >( target )->setAttribute( Qt::WA_AcceptTouchEvents );
    return new PanGesture;
}

QGestureRecognizer::Result PanGestureRecognizer::recognize( QGesture* state,
                                                            QObject* watched,
                                                            QEvent* event )
{
    PanGesture* gesture = static_cast<PanGesture *>( state );
    const QTouchEvent* touchEvent = static_cast< const QTouchEvent* >( event );

    QGestureRecognizer::Result result;

    switch( event->type( ))
    {
    case QEvent::TouchBegin:
        result = QGestureRecognizer::MayBeGesture;
        gesture->setLastOffset( QPointF( ));
        gesture->setOffset( QPointF( ));
        break;

    case QEvent::TouchEnd:
        if( gesture->state() != Qt::NoGesture )
        {
            if( touchEvent->touchPoints().size() == _nPoints )
            {
                gesture->setLastOffset( gesture->offset( ));
                QPointF offset( 0, 0 );
                QPointF position( 0, 0 );
                for( int i = 0; i < touchEvent->touchPoints().size(); ++i )
                {
                    const QTouchEvent::TouchPoint& point = touchEvent->touchPoints().at( i );
                    offset += QPointF( point.pos().x() - point.startPos().x(),
                                       point.pos().y() - point.startPos().y( ));
                    position += point.scenePos();
                }
                offset /= _nPoints;
                position /= _nPoints;
                gesture->setOffset( offset );
                gesture->setPosition( position );
            }
            result = QGestureRecognizer::FinishGesture;
        }
        else
            result = QGestureRecognizer::CancelGesture;
        break;

    case QEvent::TouchUpdate:
        if( touchEvent->touchPoints().size() == _nPoints )
        {
            gesture->setLastOffset( gesture->offset( ));
            QPointF offset( 0, 0 );
            QPointF position( 0, 0 );
            for( int i = 0; i < touchEvent->touchPoints().size(); ++i )
            {
                const QTouchEvent::TouchPoint& point = touchEvent->touchPoints().at( i );
                offset += QPointF( point.pos().x() - point.startPos().x(),
                                   point.pos().y() - point.startPos().y( ));
                position += point.scenePos();
            }
            offset /= _nPoints;
            position /= _nPoints;
            gesture->setOffset( offset );
            gesture->setPosition( position );

            if( offset.x() > 10  || offset.y() > 10 ||
                offset.x() < -10 || offset.y() < -10 )
            {
                gesture->setHotSpot( touchEvent->touchPoints().at( 0 ).startScreenPos( ));
                result = QGestureRecognizer::TriggerGesture;
            }
            else
                result = QGestureRecognizer::MayBeGesture;
        }
        break;

    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        result = QGestureRecognizer::Ignore;
        break;
    default:
        result = QGestureRecognizer::Ignore;
        break;
    }

    return result;
}

void PanGestureRecognizer::reset( QGesture* state )
{
    PanGesture* gesture = static_cast< PanGesture* >( state );
    gesture->setLastOffset( QPointF( ));
    gesture->setOffset( QPointF( ));
    gesture->setAcceleration( 0 );

    QGestureRecognizer::reset( state );
}
