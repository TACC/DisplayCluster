#include "ContentInteractionDelegate.h"

#include "ContentWindowManager.h"

#include "main.h"


ContentInteractionDelegate::ContentInteractionDelegate(ContentWindowManager *cwm)
    : contentWindowManager_(cwm)
{
}


void ContentInteractionDelegate::gestureEvent( QGestureEvent* event )
{
    contentWindowManager_->moveToFront();

    if( QGesture* gesture = event->gesture( Qt::TapAndHoldGesture ))
    {
        event->accept( Qt::TapAndHoldGesture );
        tapAndHoldUnselected( static_cast< QTapAndHoldGesture* >( gesture ));
    }

    else if( QGesture* gesture = event->gesture( PanGestureRecognizer::type( )))
    {
        event->accept( PanGestureRecognizer::type( ));
        if( contentWindowManager_->selected() )
        {
            pan( static_cast< PanGesture* >( gesture ));
        }
        else
        {
            panUnselected(static_cast< PanGesture* >( gesture ));
        }
    }

    else if( QGesture* gesture = event->gesture( Qt::PinchGesture ))
    {
        event->accept( Qt::PinchGesture );
        if( contentWindowManager_->selected() )
        {
            pinch( static_cast< QPinchGesture* >( gesture ));
        }
        else
        {
            pinchUnselected( static_cast< QPinchGesture* >( gesture ));
        }
    }

    else if( QGesture* gesture = event->gesture( DoubleTapGestureRecognizer::type( )))
    {
        event->accept( DoubleTapGestureRecognizer::type( ));
        if( contentWindowManager_->selected() )
        {
            doubleTap( static_cast< DoubleTapGesture* >( gesture ));
        }
        else
        {
            doubleTapUnselected(static_cast< DoubleTapGesture* >( gesture ));
        }
    }

    else if( QGesture* gesture = event->gesture( Qt::TapGesture ))
    {
        event->accept( Qt::TapGesture );
        if( contentWindowManager_->selected() )
        {
            tap( static_cast< QTapGesture* >( gesture ));
        }
    }

    else if( QGesture *gesture = event->gesture( Qt::SwipeGesture ))
    {
        event->accept( Qt::SwipeGesture );
        if( contentWindowManager_->selected() )
        {
            swipe( static_cast< QSwipeGesture* >( gesture ));
        }
    }
}


void ContentInteractionDelegate::tapAndHoldUnselected(QTapAndHoldGesture *gesture)
{
    if( gesture->state() == Qt::GestureFinished )
    {
        contentWindowManager_->toggleWindowState();
    }
}

void ContentInteractionDelegate::doubleTapUnselected(DoubleTapGesture *gesture)
{
    if( gesture->state() == Qt::GestureFinished )
    {
        contentWindowManager_->adjustSize( contentWindowManager_->getSizeState() == SIZE_FULLSCREEN ? SIZE_1TO1 : SIZE_FULLSCREEN );
    }
}

void ContentInteractionDelegate::panUnselected(PanGesture *gesture)
{
    const QPointF& delta = gesture->delta();
    const double dx = delta.x() / g_configuration->getTotalWidth();
    const double dy = delta.y() / g_configuration->getTotalHeight();

    if( gesture->state() == Qt::GestureStarted )
        contentWindowManager_->getContent()->blockAdvance( true );

    double x, y;
    contentWindowManager_->getPosition( x, y );
    contentWindowManager_->setPosition( x+dx, y+dy );

    if( gesture->state() == Qt::GestureCanceled ||
        gesture->state() == Qt::GestureFinished )
    {
        contentWindowManager_->getContent()->blockAdvance( false );
    }
}

void ContentInteractionDelegate::pinchUnselected(QPinchGesture *gesture)
{
    const double factor = adaptZoomFactor(gesture->scaleFactor());
    if( factor == 0.0 )
        return;

    if( gesture->state() == Qt::GestureStarted )
        contentWindowManager_->getContent()->blockAdvance( true );

    contentWindowManager_->scaleSize( factor );

    if( gesture->state() == Qt::GestureCanceled ||
        gesture->state() == Qt::GestureFinished )
    {
        contentWindowManager_->getContent()->blockAdvance( false );
    }
}

double ContentInteractionDelegate::adaptZoomFactor(double pinchGestureScaleFactor)
{
    const double factor = (pinchGestureScaleFactor - 1.0) * 0.2 + 1.0;
    if( std::isnan( factor ) || std::isinf( factor ))
        return 0.0;
    else
        return factor;
}
