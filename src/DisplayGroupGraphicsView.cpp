#include "DisplayGroupGraphicsView.h"
#include "DisplayGroupGraphicsScene.h"

DisplayGroupGraphicsView::DisplayGroupGraphicsView()
{
    // create and set scene for the view
    setScene(new DisplayGroupGraphicsScene());

    // force scene to be anchored at top left
    setAlignment(Qt::AlignLeft | Qt::AlignTop);

    // set attributes of the view
    setInteractive(true);
    setDragMode(QGraphicsView::RubberBandDrag);
    setAcceptDrops(true);
}

void DisplayGroupGraphicsView::resizeEvent(QResizeEvent * event)
{
    fitInView(sceneRect());

    QGraphicsView::resizeEvent(event);
}
