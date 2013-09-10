#include "ZoomInteractionDelegate.h"
#include "ContentWindowManager.h"
#include "main.h"

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


void ZoomInteractionDelegate::pinch(QPinchGesture *gesture)
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

