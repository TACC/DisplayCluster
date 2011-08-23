#ifndef CONTENT_GRAPHICS_ITEM_H
#define CONTENT_GRAPHICS_ITEM_H

#define CORNER_MOVE_FRACTION 0.05
#define DEFAULT_SIZE 25000.

#include <QtGui>

class Content;

class ContentGraphicsItem : public QGraphicsRectItem {

    public:

        ContentGraphicsItem(Content * parent);

    protected:

        void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
        void mouseReleaseEvent (QGraphicsSceneMouseEvent * event);
        QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    private:

        Content * parent_;

        bool resizing_;
};

#endif
