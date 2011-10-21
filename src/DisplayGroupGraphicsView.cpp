#include "DisplayGroupGraphicsView.h"
#include "DisplayGroupGraphicsScene.h"
#include "main.h"
#include <algorithm>

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
    // compute the scene rectangle to show such that the aspect ratio corresponds to the actual aspect ratio of the tiled display
    float tiledDisplayAspect = (float)g_configuration->getTotalWidth() / (float)g_configuration->getTotalHeight();
    float windowAspect = (float)width() / (float)height();

    float sceneWidth, sceneHeight;

    if(tiledDisplayAspect >= windowAspect)
    {
        sceneWidth = 1.;
        sceneHeight = tiledDisplayAspect / windowAspect;
    }
    else // tiledDisplayAspect < windowAspect
    {
        sceneHeight = 1.;
        sceneWidth = windowAspect / tiledDisplayAspect;
    }

    // make sure we have a small buffer around the (0,0,1,1) scene rectangle
    float border = 0.05;

    sceneWidth = std::max(sceneWidth, (float)1.+border);
    sceneHeight = std::max(sceneHeight, (float)1.+border);

    setSceneRect(-(sceneWidth - 1.)/2., -(sceneHeight - 1.)/2., sceneWidth, sceneHeight);

    fitInView(sceneRect());

    QGraphicsView::resizeEvent(event);
}
