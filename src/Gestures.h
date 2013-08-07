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

#ifndef GESTURES_H
#define GESTURES_H

#include <QtGui/QGesture>
#include <QtGui/QGestureRecognizer>

class PanGesture : public QGesture
{
public:
    PanGesture( QObject* parent = 0 );

    const QPointF& position() const { return _position; }
    const QPointF& lastOffset() const { return _lastOffset; }
    const QPointF& offset() const { return _offset; }
    QPointF delta() const { return _offset - _lastOffset; }
    qreal acceleration() const { return _acceleration; }

    void setPosition( const QPointF& value ) { _position = value; }
    void setLastOffset( const QPointF& value ) { _lastOffset = value; }
    void setOffset( const QPointF& value ) { _offset = value; }
    void setAcceleration( const qreal value ) { _acceleration = value; }

private:
    QPointF _position;
    QPointF _lastOffset;
    QPointF _offset;
    qreal _acceleration;
};

class DoubleTapGesture : public QGesture
{
public:
    DoubleTapGesture( QObject* parent = 0 ) : QGesture( parent ){}

    QPointF position() const { return _position; }
    void setPosition( const QPointF& pos ) { _position = pos; }

private:
    QPointF _position;
};

class PanGestureRecognizer : public QGestureRecognizer
{
public:
    PanGestureRecognizer( const int numPoints );

    virtual QGesture* create( QObject *target );

    virtual QGestureRecognizer::Result recognize( QGesture* state,
                                                  QObject* watched,
                                                  QEvent* event );

    virtual void reset( QGesture* state );

    static void install();
    static void uninstall();
    static Qt::GestureType type();

private:
    int _nPoints;
    static Qt::GestureType _type;
};

class DoubleTapGestureRecognizer : public QGestureRecognizer
{
public:
    DoubleTapGestureRecognizer();

    virtual QGesture* create( QObject *target );

    virtual QGestureRecognizer::Result recognize( QGesture* state,
                                                  QObject* watched,
                                                  QEvent* event );

    virtual void reset( QGesture* state );

    static void install();
    static void uninstall();
    static Qt::GestureType type();

private:
    QPointF _firstPoint;
    QTime _firstPointTime;
    static Qt::GestureType _type;
};

#endif
