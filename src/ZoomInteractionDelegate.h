#ifndef ZOOMINTERACTIONDELEGATE_H
#define ZOOMINTERACTIONDELEGATE_H

#include "ContentInteractionDelegate.h"

class ZoomInteractionDelegate : public ContentInteractionDelegate
{
Q_OBJECT

public:
    ZoomInteractionDelegate(ContentWindowManager *cwm);

    void pan(PanGesture *gesture);
    void pinch(QPinchGesture *gesture);

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void wheelEvent(QGraphicsSceneWheelEvent *event);
};

#endif // ZOOMINTERACTIONDELEGATE_H
