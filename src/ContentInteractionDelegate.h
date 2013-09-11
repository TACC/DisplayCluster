#ifndef CONTENTINTERACTIONDELEGATE_H
#define CONTENTINTERACTIONDELEGATE_H

#include "Gestures.h"

#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QObject>

class ContentWindowManager;

class ContentInteractionDelegate : public QObject
{
Q_OBJECT

public:

    ContentInteractionDelegate(ContentWindowManager* cwm);

    // Main entry point for gesture events
    void gestureEvent( QGestureEvent *event );

    // Virtual touch gestures
    virtual void tap( QTapGesture* gesture ) {}
    virtual void doubleTap( DoubleTapGesture* gesture ) {}
    virtual void pan( PanGesture* gesture ) {}
    virtual void swipe( QSwipeGesture *gesture ) {}
    virtual void pinch( QPinchGesture* gesture ) {}
    //virtual void tapAndHold( QTapAndHoldGesture* gesture ) {}

    // Keyboard + Mouse input
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * event) {}
    virtual void mousePressEvent(QGraphicsSceneMouseEvent * event) {}
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event) {}
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event) {}
    virtual void wheelEvent(QGraphicsSceneWheelEvent * event) {}
    virtual void keyPressEvent(QKeyEvent *event) {}
    virtual void keyReleaseEvent(QKeyEvent *event) {}

protected:

    ContentWindowManager* contentWindowManager_;

    double adaptZoomFactor(double pinchGestureScaleFactor);

private:

    // Touch gestures when ContentWindowManager is not in interaction mode
    void doubleTapUnselected( DoubleTapGesture* gesture );
    void tapAndHoldUnselected( QTapAndHoldGesture* gesture );
    void panUnselected( PanGesture* gesture );
    void pinchUnselected( QPinchGesture* gesture );

};

#endif // CONTENTINTERACTIONDELEGATE_H
