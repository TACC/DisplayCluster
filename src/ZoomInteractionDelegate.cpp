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

#include "ZoomInteractionDelegate.h"
#include "ContentWindowManager.h"
#include "globals.h"
#include "Configuration.h"
#include "gestures/PanGesture.h"
#include "gestures/PinchGesture.h"

ZoomInteractionDelegate::ZoomInteractionDelegate(ContentWindowManager* cwm)
    : ContentInteractionDelegate(cwm)
{
}

void ZoomInteractionDelegate::pan(PanGesture *gesture)
{
    const QPointF& delta = gesture->delta();
    const double dx = delta.x() / g_configuration->getTotalWidth();
    const double dy = delta.y() / g_configuration->getTotalHeight();

    double zoom = contentWindowManager_->getZoom();
    double centerX, centerY;
    contentWindowManager_->getCenter(centerX, centerY);
    centerX = centerX - 2.*dx / zoom;
    centerY = centerY - 2.*dy / zoom;
    contentWindowManager_->setCenter(centerX, centerY);
}


void ZoomInteractionDelegate::pinch(PinchGesture *gesture)
{
    const double factor = adaptZoomFactor(gesture->scaleFactor());
    if( factor == 0.0 )
        return;

    contentWindowManager_->setZoom( contentWindowManager_->getZoom() * factor );
}


void ZoomInteractionDelegate::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    // handle zooms / pans
    QPointF delta = event->scenePos() - event->lastScenePos();

    if(event->buttons().testFlag(Qt::RightButton))
    {
        // increment zoom

        // if this is a touch event, use cross-product for determining change in zoom (counterclockwise rotation == zoom in, etc.)
        // otherwise, use y as the change in zoom
        double zoomDelta;

        if(event->modifiers().testFlag(Qt::AltModifier))
        {
            zoomDelta = (event->scenePos().x()-0.5) * delta.y() - (event->scenePos().y()-0.5) * delta.x();
            zoomDelta *= 2.;
        }
        else
        {
            zoomDelta = delta.y();
        }

        double zoom = contentWindowManager_->getZoom() * (1. - zoomDelta);

        contentWindowManager_->setZoom(zoom);
    }
    else if(event->buttons().testFlag(Qt::LeftButton))
    {
        // pan (move center coordinates)
        double zoom = contentWindowManager_->getZoom();
        double centerX, centerY;
        contentWindowManager_->getCenter(centerX, centerY);

        centerX = centerX + 2.*delta.x() / zoom;
        centerY = centerY + 2.*delta.y() / zoom;

        contentWindowManager_->setCenter(centerX, centerY);
    }
}

void ZoomInteractionDelegate::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    // change zoom based on wheel delta
    // deltas are counted in 1/8 degrees. so, scale based on 180 degrees => delta = 180*8 = 1440
    double zoomDelta = (double)event->delta() / 1440.;
    double zoom = contentWindowManager_->getZoom() * (1. + zoomDelta);

    contentWindowManager_->setZoom(zoom);
}

