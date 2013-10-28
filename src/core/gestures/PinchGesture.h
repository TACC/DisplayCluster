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

#ifndef PINCHGESTURE_H
#define PINCHGESTURE_H

#include <QtGui/QGesture>

/**
 * This class defines a pinch gesture. The implementation enhances the Qt
 * shipped PinchGesture to setup a normalized center point which is required in
 * the event handling of this application.
 * @sa QPinchGesture
 */
class PinchGesture : public QGesture
{
public:
    /** @sa QGesture */
    PinchGesture( QObject* parent = 0 );

    /** @sa QPinchGesture::ChangeFlags */
    enum ChangeFlags
    {
        NothingChanged = 0x0,
        ScaleFactorChanged = 0x1,
        RotationAngleChanged = 0x2,
        CenterPointChanged = 0x4
    };

    /** @sa QPinchGesture::totalChangeFlags */
    ChangeFlags totalChangeFlags() const { return _totalChangeFlags; }

    /** @sa QPinchGesture::setTotalChangeFlags */
    void setTotalChangeFlags(ChangeFlags value) { _totalChangeFlags = value; }

    /** @sa QPinchGesture::changeFlags */
    ChangeFlags changeFlags() const { return _changeFlags; }

    /** @sa QPinchGesture::setChangeFlags */
    void setChangeFlags(ChangeFlags value) { _changeFlags = value; }

    /** @sa QPinchGesture::startCenterPoint */
    QPointF startCenterPoint() const { return _startCenterPoint; }

    /** @sa QPinchGesture::lastCenterPoint */
    QPointF lastCenterPoint() const { return _lastCenterPoint; }

    /** @sa QPinchGesture::centerPoint */
    QPointF centerPoint() const { return _centerPoint; }

    /** @return the normalized center position of the pinch */
    QPointF normalizedCenterPoint() const { return _normalizedCenterPoint; }

    /** @sa QPinchGesture::setStartCenterPoint */
    void setStartCenterPoint(const QPointF &value) { _startCenterPoint = value; }

    /** @sa QPinchGesture::setLastCenterPoint */
    void setLastCenterPoint(const QPointF &value) { _lastCenterPoint = value; }

    /** @sa QPinchGesture::setCenterPoint */
    void setCenterPoint(const QPointF &value) { _centerPoint = value; }

    /** Set the normalized center position of the pinch */
    void setNormalizedCenterPoint(const QPointF &value) { _normalizedCenterPoint = value; }

    /** @sa QPinchGesture::totalScaleFactor */
    qreal totalScaleFactor() const { return _totalScaleFactor; }

    /** @sa QPinchGesture::lastScaleFactor */
    qreal lastScaleFactor() const { return _lastScaleFactor; }

    /** @sa QPinchGesture::scaleFactor */
    qreal scaleFactor() const { return _scaleFactor; }

    /** @sa QPinchGesture::setTotalScaleFactor */
    void setTotalScaleFactor(qreal value) { _totalScaleFactor = value; }

    /** @sa QPinchGesture::setLastScaleFactor */
    void setLastScaleFactor(qreal value) { _lastScaleFactor = value; }

    /** @sa QPinchGesture::setScaleFactor */
    void setScaleFactor(qreal value) { _scaleFactor = value; }

    /** @sa QPinchGesture::totalRotationAngle */
    qreal totalRotationAngle() const { return _totalRotationAngle; }

    /** @sa QPinchGesture::lastRotationAngle */
    qreal lastRotationAngle() const { return _lastRotationAngle; }

    /** @sa QPinchGesture::rotationAngle */
    qreal rotationAngle() const { return _rotationAngle; }

    /** @sa QPinchGesture::setTotalRotationAngle */
    void setTotalRotationAngle(qreal value) { _totalRotationAngle = value; }

    /** @sa QPinchGesture::setLastRotationAngle */
    void setLastRotationAngle(qreal value) { _lastRotationAngle = value; }

    /** @sa QPinchGesture::setRotationAngle */
    void setRotationAngle(qreal value) { _rotationAngle = value; }

private:
    friend class PinchGestureRecognizer;

    ChangeFlags _totalChangeFlags;
    ChangeFlags _changeFlags;

    QPointF _startCenterPoint;
    QPointF _lastCenterPoint;
    QPointF _centerPoint;
    QPointF _normalizedCenterPoint;

    qreal _totalScaleFactor;
    qreal _lastScaleFactor;
    qreal _scaleFactor;

    qreal _totalRotationAngle;
    qreal _lastRotationAngle;
    qreal _rotationAngle;

    bool _isNewSequence;
    QPointF _startPosition[2];
};

#endif
