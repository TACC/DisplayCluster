#include "DisplayGroupGraphicsView.h"

DisplayGroupGraphicsView::DisplayGroupGraphicsView()
{
    // create and set scene for the view
    setScene(new QGraphicsScene(0., 0., 100000., 100000.));

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
