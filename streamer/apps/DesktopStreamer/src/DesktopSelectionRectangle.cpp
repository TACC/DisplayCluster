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

#include <iostream>

#include "DesktopSelectionRectangle.h"
#include "main.h"

DesktopSelectionRectangle::DesktopSelectionRectangle()
{
    // current coordinates from MainWindow
    g_mainWindow->getCoordinates(x_, y_, width_, height_);

    // graphics items are movable
    setFlag(QGraphicsItem::ItemIsMovable, true);

    // default pen
    setPen(QPen(QBrush(QColor(255, 0, 0)), PEN_WIDTH));

    // current coordinates, accounting for width of the pen outline
    setRect(x_, y_, width_, height_);
}

void
DesktopSelectionRectangle::reset()
{
		auto screens = QGuiApplication::screens();
		auto screenSize = screens[0]->size();
		auto w = screenSize.width();
		auto h = screenSize.height();
    setRect(0, 0, w, h);
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

    setRect(x_, y_, width_, height_);
}

void DesktopSelectionRectangle::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    if(event->buttons().testFlag(Qt::LeftButton) == true)
    {
        if(g_mainWindow->isResizing())
        {
            QRectF r = rect();
            QPointF eventPos = event->pos();

						if (selectedCorner_ == LL)
								r.setBottomLeft(eventPos);
						else if (selectedCorner_ == UL)
								r.setTopLeft(eventPos);
						else if (selectedCorner_ == LR)
								r.setBottomRight(eventPos);
						else if (selectedCorner_ == UR)
								r.setTopRight(eventPos);

            setRect(r);
						g_desktopSelectionWindow->placeButton(r.x(), r.y());
        }
        else
        {
            QRectF r = rect();

						// Desktop geometry
						int dtw, dth;

						auto screens = QGuiApplication::screens();
						auto screenSize = screens[0]->size();
						dtw = screenSize.width();
						dth = screenSize.height();

            QPointF delta = event->pos() - event->lastPos();

						auto xl = r.x() + delta.x();
						if (xl < 0) xl = 0;
						else if (xl > dtw) xl = dtw;

						auto xr = r.x() + r.width() + delta.x();
						if (xr < 0) xr = 0;
						else if (xr > dtw) xr = dtw;

						auto yb = r.y() + delta.y();
						if (yb < 0) yb = 0;
						else if (yb > dth) yb = dth;

						auto yt = r.y() + r.height() + delta.y();
						if (yt < 0) yt = 0;
						else if (yt > dth) yt = dth;

						setRect(xl, yb, xr-xl, yt-yb);
						g_desktopSelectionWindow->placeButton(r.x(), r.y());
        }

        updateCoordinates();
    }
}

void DesktopSelectionRectangle::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    // item rectangle and event position
    QRectF r = rect();
    QPointF eventPos = event->pos();

		selectedCorner_ = NONE;

    if (fabs(r.x() - eventPos.x()) <= CORNER_RESIZE_THRESHHOLD)
		{
			if (fabs(r.y() - eventPos.y()) <= CORNER_RESIZE_THRESHHOLD) { selectedCorner_ = UL; }
			else if (fabs((r.y()+r.height()) - eventPos.y()) <= CORNER_RESIZE_THRESHHOLD) { selectedCorner_ = LL; }
		}
		else if (fabs((r.x() + r.width()) - eventPos.x()) <= CORNER_RESIZE_THRESHHOLD)
		{
			if (fabs(r.y() - eventPos.y()) <= CORNER_RESIZE_THRESHHOLD) { selectedCorner_ = UR; }
			else if (fabs((r.y()+r.height()) - eventPos.y()) <= CORNER_RESIZE_THRESHHOLD) { selectedCorner_ = LR; }
		}
			
    // check to see if user clicked on the resize button
    if(selectedCorner_ != NONE)
				g_mainWindow->setResizing();

    QGraphicsItem::mousePressEvent(event);
}

void DesktopSelectionRectangle::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
		g_mainWindow->clearResizing();
		g_mainWindow->updateCoordinates();
    QGraphicsItem::mouseReleaseEvent(event);
}

void DesktopSelectionRectangle::updateCoordinates()
{
    QRectF sceneRect = mapRectToScene(rect());

#if 0
    x_ = (int)sceneRect.x() + PEN_WIDTH/2;
    y_ = (int)sceneRect.y() + PEN_WIDTH/2;
    width_ = (int)sceneRect.width() - PEN_WIDTH;
    height_ = (int)sceneRect.height() - PEN_WIDTH;

    g_mainWindow->setCoordinates(x_, y_, width_, height_);
#else
    x_ = sceneRect.x();
    y_ = sceneRect.y();
    width_ = sceneRect.width();
    height_ = sceneRect.height();

    g_mainWindow->setCoordinates(x_, y_, width_, height_);
#endif
}
