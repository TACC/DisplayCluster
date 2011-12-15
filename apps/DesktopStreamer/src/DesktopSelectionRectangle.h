#ifndef DESKTOP_SELECTION_RECTANGLE_H
#define DESKTOP_SELECTION_RECTANGLE_H

#define PEN_WIDTH 10 // should be even
#define CORNER_RESIZE_THRESHHOLD 50

#include <QtGui>

class DesktopSelectionRectangle : public QGraphicsRectItem {

    public:

        DesktopSelectionRectangle();

        // QGraphicsRectItem painting
        void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget=0);

        void setCoordinates(int x, int y, int width, int height);

    protected:

        // QGraphicsRectItem events
        void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
        void mousePressEvent(QGraphicsSceneMouseEvent * event);
        void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);

    private:

        void updateCoordinates();

        // resizing state
        bool resizing_;

        int x_, y_, width_, height_;
};

#endif
