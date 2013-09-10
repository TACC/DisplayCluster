#ifndef PIXELSTREAMINTERACTIONDELEGATE_H
#define PIXELSTREAMINTERACTIONDELEGATE_H

#include "ContentInteractionDelegate.h"

struct InteractionState;

class PixelStreamInteractionDelegate : public ContentInteractionDelegate
{
Q_OBJECT

public:
    PixelStreamInteractionDelegate(ContentWindowManager *cwm);

    virtual void swipe( QSwipeGesture *gesture );
    virtual void pan( PanGesture* gesture) ;
    virtual void pinch( QPinchGesture* gesture );
    virtual void doubleTap( DoubleTapGesture* gesture );
    virtual void tap( QTapGesture* gesture );

    // Keyboard + Mouse input
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
    virtual void mousePressEvent(QGraphicsSceneMouseEvent * event);
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
    virtual void wheelEvent(QGraphicsSceneWheelEvent * event);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);

private:
    QPointF mousePressPos_;

    template <typename T>
    InteractionState getMouseInteractionState(const T *mouseEvent);
    void setMouseMoveNormalizedDelta(const QGraphicsSceneMouseEvent *event, InteractionState &interactionState);

    template<typename T>
    InteractionState getGestureInteractionState(const T *gesture);
    InteractionState getGestureInteractionState(const QPinchGesture *gesture);
    void setPanGesureNormalizedDelta(const PanGesture *gesture, InteractionState &interactionState);
};

#endif // PIXELSTREAMINTERACTIONDELEGATE_H
