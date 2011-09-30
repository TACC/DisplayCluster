#ifndef CONTENT_GRAPHICS_ITEM_H
#define CONTENT_GRAPHICS_ITEM_H

#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

class Content;

class ContentGraphicsItem : public QGraphicsRectItem {

    public:

        ContentGraphicsItem(boost::shared_ptr<Content> parent);

        void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget=0);

    protected:

        void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
        void mousePressEvent(QGraphicsSceneMouseEvent * event);
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event);
        void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
        QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    private:

        bool initialized_;

        bool resizing_;
        bool selected_;

        Qt::MouseButton button_;

        boost::weak_ptr<Content> parent_;

        void updateParent();
        void getButtonDimensions(float &width, float &height);
};

#endif
