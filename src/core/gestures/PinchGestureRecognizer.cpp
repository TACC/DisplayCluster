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

#include "PinchGestureRecognizer.h"
#include "PinchGesture.h"

#include <QtGui/QTouchEvent>
#include <QtGui/QWidget>

Qt::GestureType PinchGestureRecognizer::_type = Qt::CustomGesture;

void PinchGestureRecognizer::install()
{
    _type = QGestureRecognizer::registerRecognizer( new PinchGestureRecognizer );
}

void PinchGestureRecognizer::uninstall()
{
    QGestureRecognizer::unregisterRecognizer( _type );
}

Qt::GestureType PinchGestureRecognizer::type()
{
    return _type;
}

PinchGestureRecognizer::PinchGestureRecognizer()
{
}

QGesture* PinchGestureRecognizer::create( QObject* target )
{
    if( target && target->isWidgetType( ))
        static_cast< QWidget* >( target )->setAttribute( Qt::WA_AcceptTouchEvents );
    return new PinchGesture;
}

QGestureRecognizer::Result PinchGestureRecognizer::recognize( QGesture* state,
                                                            QObject* watched,
                                                            QEvent* event )
{
    PinchGesture* gesture = static_cast<PinchGesture *>( state );

    const QTouchEvent *ev = static_cast<const QTouchEvent *>(event);

    QGestureRecognizer::Result result;

    switch (event->type()) {
    case QEvent::TouchBegin: {
        result = QGestureRecognizer::MayBeGesture;
        break;
    }
    case QEvent::TouchEnd: {
        if (gesture->state() != Qt::NoGesture) {
            result = QGestureRecognizer::FinishGesture;
        } else {
            result = QGestureRecognizer::CancelGesture;
        }
        break;
    }
    case QEvent::TouchUpdate: {
        gesture->_changeFlags = PinchGesture::NothingChanged;
        if (ev->touchPoints().size() == 2) {
            QTouchEvent::TouchPoint p1 = ev->touchPoints().at(0);
            QTouchEvent::TouchPoint p2 = ev->touchPoints().at(1);

            gesture->setHotSpot( p1.screenPos( ));

            QPointF centerPoint = (p1.screenPos() + p2.screenPos()) / 2.0;
            if (gesture->_isNewSequence) {
                gesture->_startPosition[0] = p1.screenPos();
                gesture->_startPosition[1] = p2.screenPos();
                gesture->_lastCenterPoint = centerPoint;
            } else {
                gesture->_lastCenterPoint = gesture->_centerPoint;
            }
            gesture->_centerPoint = centerPoint;
            gesture->_normalizedCenterPoint = (p1.normalizedPos() + p2.normalizedPos()) / 2.0;;

            gesture->_changeFlags = static_cast< PinchGesture::ChangeFlags >(gesture->_changeFlags | PinchGesture::CenterPointChanged);

            if (gesture->_isNewSequence) {
                gesture->_scaleFactor = 1.0;
                gesture->_lastScaleFactor = 1.0;
            } else {
                gesture->_lastScaleFactor = gesture->_scaleFactor;
                QLineF line(p1.screenPos(), p2.screenPos());
                QLineF lastLine(p1.lastScreenPos(),  p2.lastScreenPos());
                gesture->_scaleFactor = line.length() / lastLine.length();
            }
            gesture->_totalScaleFactor = gesture->_totalScaleFactor * gesture->_scaleFactor;
            gesture->_changeFlags = static_cast< PinchGesture::ChangeFlags >(gesture->_changeFlags | PinchGesture::ScaleFactorChanged);

            qreal angle = QLineF(p1.screenPos(), p2.screenPos()).angle();
            if (angle > 180)
                angle -= 360;
            qreal startAngle = QLineF(p1.startScreenPos(), p2.startScreenPos()).angle();
            if (startAngle > 180)
                startAngle -= 360;
            const qreal rotationAngle = startAngle - angle;
            if (gesture->_isNewSequence)
                gesture->_lastRotationAngle = 0.0;
            else
                gesture->_lastRotationAngle = gesture->_rotationAngle;
            gesture->_rotationAngle = rotationAngle;
            gesture->_totalRotationAngle += gesture->_rotationAngle - gesture->_lastRotationAngle;
            gesture->_changeFlags = static_cast< PinchGesture::ChangeFlags >(gesture->_changeFlags | PinchGesture::RotationAngleChanged);
            gesture->_totalChangeFlags = static_cast< PinchGesture::ChangeFlags >(gesture->_totalChangeFlags | gesture->_changeFlags);
            gesture->_isNewSequence = false;
            result = QGestureRecognizer::TriggerGesture;
        } else {
            gesture->_isNewSequence = true;
            if (gesture->state() == Qt::NoGesture)
                result = QGestureRecognizer::Ignore;
            else
                result = QGestureRecognizer::FinishGesture;
        }
        break;
    }
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

void PinchGestureRecognizer::reset( QGesture* state )
{
    PinchGesture *gesture = static_cast<PinchGesture *>(state);

    gesture->_totalChangeFlags = gesture->_changeFlags = PinchGesture::NothingChanged;

    gesture->_startCenterPoint = gesture->_lastCenterPoint = gesture->_centerPoint = QPointF();
    gesture->_totalScaleFactor = gesture->_lastScaleFactor = gesture->_scaleFactor = 1;
    gesture->_totalRotationAngle = gesture->_lastRotationAngle = gesture->_rotationAngle = 0;

    gesture->_isNewSequence = true;
    gesture->_startPosition[0] = gesture->_startPosition[1] = QPointF();

    QGestureRecognizer::reset(state);
}
