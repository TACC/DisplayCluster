#ifndef DISPLAY_GROUP_GRAPHICS_SCENE_H
#define DISPLAY_GROUP_GRAPHICS_SCENE_H

#include <QtGui>
#include <boost/shared_ptr.hpp>

class Marker;

class DisplayGroupGraphicsScene : public QGraphicsScene {

    public:

        DisplayGroupGraphicsScene();

    protected:

        void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
        void mousePressEvent(QGraphicsSceneMouseEvent * event);
        void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);

    private:

        boost::shared_ptr<Marker> marker_;
};

#endif
