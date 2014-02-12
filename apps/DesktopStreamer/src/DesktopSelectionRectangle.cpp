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

#include "DesktopSelectionRectangle.h"

#define PEN_WIDTH 10 // should be even
#define CORNER_RESIZE_THRESHHOLD 50
#define DEFAULT_SIZE 100

DesktopSelectionRectangle::DesktopSelectionRectangle()
    : resizing_(false)
    , x_(0)
    , y_(0)
    , width_(DEFAULT_SIZE)
    , height_(DEFAULT_SIZE)
{
    // graphics items are movable
    setFlag(QGraphicsItem::ItemIsMovable, true);

    // default pen
    setPen(QPen(QBrush(QColor(255, 0, 0)), PEN_WIDTH));

    // current coordinates, accounting for width of the pen outline
    setRect(x_-PEN_WIDTH/2, y_-PEN_WIDTH/2, width_+PEN_WIDTH, height_+PEN_WIDTH);
}

void DesktopSelectionRectangle::paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget)
{
    QGraphicsRectItem::paint(painter, option, widget);
}

void DesktopSelectionRectangle::setCoordinates(int x, int y, int width, int height)
{
    x_ = x;
    y_ = y;
    width_ = width;
    height_ = height;

    setRect(mapRectFromScene(x_-PEN_WIDTH/2, y_-PEN_WIDTH/2, width_+PEN_WIDTH, height_+PEN_WIDTH));
}

void DesktopSelectionRectangle::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    if(event->buttons().testFlag(Qt::LeftButton))
    {
        if(resizing_)
        {
            QRectF r = rect();
            QPointF eventPos = event->pos();

            r.setBottomRight(eventPos);

            setRect(r);
        }
        else
        {
            QPointF delta = event->pos() - event->lastPos();

            moveBy(delta.x(), delta.y());
        }

        updateCoordinates();
        emit coordinatesChanged(x_, y_, width_, height_);
    }
}

void DesktopSelectionRectangle::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    // item rectangle and event position
    QRectF r = rect();
    QPointF eventPos = event->pos();

    // check to see if user clicked on the resize button
    if(fabs((r.x()+r.width()) - eventPos.x()) <= CORNER_RESIZE_THRESHHOLD && fabs((r.y()+r.height()) - eventPos.y()) <= CORNER_RESIZE_THRESHHOLD)
    {
        resizing_ = true;
    }

    QGraphicsItem::mousePressEvent(event);
}

void DesktopSelectionRectangle::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    resizing_ = false;

    QGraphicsItem::mouseReleaseEvent(event);
}

void DesktopSelectionRectangle::updateCoordinates()
{
    QRectF sceneRect = mapRectToScene(rect());

    x_ = (int)sceneRect.x() + PEN_WIDTH/2;
    y_ = (int)sceneRect.y() + PEN_WIDTH/2;
    width_ = (int)sceneRect.width() - PEN_WIDTH;
    height_ = (int)sceneRect.height() - PEN_WIDTH;
}
