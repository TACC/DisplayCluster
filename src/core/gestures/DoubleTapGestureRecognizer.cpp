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

#include "DoubleTapGestureRecognizer.h"
#include "DoubleTapGesture.h"

#include <QtGui/QTouchEvent>
#include <QtGui/QWidget>


Qt::GestureType DoubleTapGestureRecognizer::_type = Qt::CustomGesture;

void DoubleTapGestureRecognizer::install()
{
    _type = QGestureRecognizer::registerRecognizer( new DoubleTapGestureRecognizer );
}

void DoubleTapGestureRecognizer::uninstall()
{
    QGestureRecognizer::unregisterRecognizer( _type );
}

Qt::GestureType DoubleTapGestureRecognizer::type()
{
    return _type;
}

DoubleTapGestureRecognizer::DoubleTapGestureRecognizer()
    : _firstPoint( -1, -1 )
{
}

QGesture* DoubleTapGestureRecognizer::create( QObject* target )
{
    if( target && target->isWidgetType( ))
        static_cast< QWidget* >( target )->setAttribute( Qt::WA_AcceptTouchEvents );
    return new DoubleTapGesture;
}

QGestureRecognizer::Result DoubleTapGestureRecognizer::recognize( QGesture* state,
                                                            QObject* watched,
                                                            QEvent* event )
{
    const QTouchEvent* touchEvent = static_cast< const QTouchEvent* >( event );
    DoubleTapGesture* gesture = static_cast<DoubleTapGesture *>(state);

    enum { TapRadius = 40 };

    switch( event->type( ))
    {
    case QEvent::TouchBegin:
        if( touchEvent->touchPoints().size() != 1 )
            return QGestureRecognizer::Ignore;
        return QGestureRecognizer::MayBeGesture;

    case QEvent::TouchEnd:
    {
        if( touchEvent->touchPoints().size() != 1 )
            return QGestureRecognizer::Ignore;
        const QTouchEvent::TouchPoint& p = touchEvent->touchPoints().at(0);
        QPoint delta = p.pos().toPoint() - p.startPos().toPoint();
        if( delta.manhattanLength() > TapRadius )
            return QGestureRecognizer::CancelGesture;

        if( _firstPoint != QPointF( -1, -1 ))
        {
            delta =  p.pos().toPoint() - _firstPoint.toPoint();
            if( delta.manhattanLength() > TapRadius ||
                _firstPointTime.elapsed() > 750 )
            {
                _firstPoint = QPointF( -1, -1 );
                return QGestureRecognizer::CancelGesture;
            }

            gesture->setPosition( p.startScreenPos());
            gesture->setHotSpot( gesture->position( ));
            _firstPoint = QPointF( -1, -1 );
            return QGestureRecognizer::FinishGesture;
        }
        else
        {
            _firstPoint = p.pos();
            _firstPointTime.restart();
            return QGestureRecognizer::MayBeGesture;
        }
    }

    case QEvent::TouchUpdate:
        if( touchEvent->touchPoints().size() != 1 )
            return QGestureRecognizer::Ignore;
        return QGestureRecognizer::MayBeGesture;

    default:
        return QGestureRecognizer::Ignore;
    }
}

void DoubleTapGestureRecognizer::reset( QGesture* state )
{
    QGestureRecognizer::reset( state );
}
