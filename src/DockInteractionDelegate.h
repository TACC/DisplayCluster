#ifndef DOCKINTERACTIONDELEGATE_H
#define DOCKINTERACTIONDELEGATE_H

#include "ContentInteractionDelegate.h"

class DockInteractionDelegate : public ContentInteractionDelegate
{
Q_OBJECT

public:
    DockInteractionDelegate(ContentWindowManager *cwm);

    virtual void swipe( QSwipeGesture *gesture );
    virtual void pan( PanGesture* gesture) ;
    virtual void pinch( QPinchGesture* gesture );
    virtual void doubleTap( DoubleTapGesture* gesture );
    virtual void tap( QTapGesture* gesture );

};

#endif // DOCKINTERACTIONDELEGATE_H
