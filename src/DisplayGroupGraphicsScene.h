#ifndef DISPLAY_GROUP_GRAPHICS_SCENE_H
#define DISPLAY_GROUP_GRAPHICS_SCENE_H

#include <QtGui>

class DisplayGroupGraphicsScene : public QGraphicsScene {

    public:

        DisplayGroupGraphicsScene();

    protected:

        void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
        void mousePressEvent(QGraphicsSceneMouseEvent * event);
        void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);

    private:

        void updateCursor(float x, float y);
};

#endif
