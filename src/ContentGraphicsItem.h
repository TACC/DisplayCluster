#ifndef CONTENT_GRAPHICS_ITEM_H
#define CONTENT_GRAPHICS_ITEM_H

#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

class Content;

class ContentGraphicsItem : public QGraphicsRectItem {

    public:

        ContentGraphicsItem(boost::shared_ptr<Content> parent);

    protected:

        void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
        void mouseReleaseEvent (QGraphicsSceneMouseEvent * event);
        QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    private:

        bool resizing_;

        boost::weak_ptr<Content> parent_;

        void updateParent();
};

#endif
