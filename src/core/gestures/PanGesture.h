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

#ifndef PANGESTURE_H
#define PANGESTURE_H

#include <QtGui/QGesture>

/**
 * This class defines a pan gesture. The implementation enhances the Qt
 * shipped PanGesture to setup a normalized position which is required in
 * the event handling of this application.
 * @sa QPanGesture
 */
class PanGesture : public QGesture
{
public:
    /** @sa QGesture */
    PanGesture( QObject* parent = 0 );

    /** @return the normalized center position of the pan */
    const QPointF& position() const { return _position; }

    /** @sa QPanGesture::lastOffset */
    const QPointF& lastOffset() const { return _lastOffset; }

    /** @sa QPanGesture::offset */
    const QPointF& offset() const { return _offset; }

    /** @sa QPanGesture::delta */
    QPointF delta() const { return _offset - _lastOffset; }

    /** @sa QPanGesture::acceleration */
    qreal acceleration() const { return _acceleration; }

    /** Set the normalized center position of the pan */
    void setPosition( const QPointF& value ) { _position = value; }

    /** @sa QPanGesture::setLastOffset */
    void setLastOffset( const QPointF& value ) { _lastOffset = value; }

    /** @sa QPanGesture::setOffset */
    void setOffset( const QPointF& value ) { _offset = value; }

    /** @sa QPanGesture::setAcceleration */
    void setAcceleration( const qreal value ) { _acceleration = value; }

private:
    QPointF _position;
    QPointF _lastOffset;
    QPointF _offset;
    qreal _acceleration;
};

#endif
