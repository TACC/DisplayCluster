#include "DockInteractionDelegate.h"
#include "main.h"
#include "ContentWindowManager.h"

#include "Dock.h"
#include "Pictureflow.h"

DockInteractionDelegate::DockInteractionDelegate(ContentWindowManager* cwm)
    : ContentInteractionDelegate(cwm)
{
}


void DockInteractionDelegate::swipe(QSwipeGesture *gesture)
{

}

void DockInteractionDelegate::pan(PanGesture *gesture)
{
    const QPointF& delta = gesture->delta();

    const int offs = delta.x()/4;
    g_dock->getFlow()->showSlide( g_dock->getFlow()->centerIndex() - offs );
}

void DockInteractionDelegate::pinch(QPinchGesture *gesture)
{

}

void DockInteractionDelegate::doubleTap(DoubleTapGesture *gesture)
{

}

void DockInteractionDelegate::tap(QTapGesture *gesture)
{
    if( gesture->state() == Qt::GestureFinished )
    {
        double x_, y_, w_, h_;
        contentWindowManager_->getCoordinates(x_, y_, w_, h_);

        const int xPos = gesture->position().x() - (x_*g_configuration->getTotalWidth());
        const int mid = (w_*g_configuration->getTotalWidth())/2;
        const int slideMid = g_dock->getFlow()->slideSize().width()/2;

        if( xPos > mid-slideMid && xPos < mid+slideMid )
        {
            g_dock->onItem();
        }
        else
        {
            if( xPos > mid )
              g_dock->getFlow()->showNext();
            else
              g_dock->getFlow()->showPrevious();
        }
    }
}
