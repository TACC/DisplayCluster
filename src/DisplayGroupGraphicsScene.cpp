#include "DisplayGroupGraphicsScene.h"
#include "main.h"
#include "Marker.h"

DisplayGroupGraphicsScene::DisplayGroupGraphicsScene()
{
    setSceneRect(0., 0., 1., 1.);
}

void DisplayGroupGraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent * event)
{
    updateMarker(event->scenePos().x(), event->scenePos().y());

    QGraphicsScene::mouseMoveEvent(event);
}

void DisplayGroupGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent * event)
{
    updateMarker(event->scenePos().x(), event->scenePos().y());

    QGraphicsScene::mousePressEvent(event);
}

void DisplayGroupGraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent * event)
{
    updateMarker(event->scenePos().x(), event->scenePos().y());

    QGraphicsScene::mouseReleaseEvent(event);
}

void DisplayGroupGraphicsScene::updateMarker(float x, float y)
{
    g_displayGroup->getMarker().setPosition(x, y);
    g_displayGroup->synchronize(); // todo: only call this if an item isn't underneath (x,y)
}
