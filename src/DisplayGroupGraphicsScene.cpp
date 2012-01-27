#include "DisplayGroupGraphicsScene.h"
#include "main.h"
#include "Marker.h"

DisplayGroupGraphicsScene::DisplayGroupGraphicsScene()
{
    setSceneRect(0., 0., 1., 1.);

    // for the tiled display
    addRect(0., 0., 1., 1.);

    // add rectangles for the tiles
    refreshTileRects();

    // get marker for this scene
    marker_ = g_displayGroupManager->getNewMarker();
}

void DisplayGroupGraphicsScene::refreshTileRects()
{
    // add rectangles for tiled display and each monitor

    // tiled display parameters
    int numTilesWidth = g_configuration->getNumTilesWidth();
    int screenWidth = g_configuration->getScreenWidth();
    int mullionWidth = g_configuration->getMullionWidth();

    int numTilesHeight = g_configuration->getNumTilesHeight();
    int screenHeight = g_configuration->getScreenHeight();
    int mullionHeight = g_configuration->getMullionHeight();

    int totalWidth = g_configuration->getTotalWidth();
    int totalHeight = g_configuration->getTotalHeight();

    // rendering parameters

    // border
    QPen pen;
    pen.setColor(QColor(0,0,0));

    // fill color / opacity
    QBrush brush = QBrush(QColor(0, 0, 0, 32));

    // clear existing tile rects
    for(unsigned int i=0; i<tileRects_.size(); i++)
    {
        delete tileRects_[i];
    }

    tileRects_.clear();

    for(int i=0; i<numTilesWidth; i++)
    {
        for(int j=0; j<numTilesHeight; j++)
        {
            // border calculations
            double left = (double)i / (double)numTilesWidth * ( (double)numTilesWidth * (double)screenWidth ) + (double)i * (double)mullionWidth;
            double right = left + (double)screenWidth;
            double bottom = j / (double)numTilesHeight * ( (double)numTilesHeight * (double)screenHeight ) + (double)j * (double)mullionHeight;
            double top = bottom + (double)screenHeight;

            // normalize to 0->1
            left /= (double)totalWidth;
            right /= (double)totalWidth;
            bottom /= (double)totalHeight;
            top /= (double)totalHeight;

            tileRects_.push_back(addRect(left, bottom, right-left, top-bottom, pen, brush));
        }
    }
}

void DisplayGroupGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    marker_->setPosition(event->scenePos().x(), event->scenePos().y());

    QGraphicsScene::mouseMoveEvent(event);
}

void DisplayGroupGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    marker_->setPosition(event->scenePos().x(), event->scenePos().y());

    QGraphicsScene::mousePressEvent(event);
}

void DisplayGroupGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    marker_->setPosition(event->scenePos().x(), event->scenePos().y());

    QGraphicsScene::mouseReleaseEvent(event);
}
