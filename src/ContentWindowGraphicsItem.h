#ifndef CONTENT_WINDOW_GRAPHICS_ITEM_H
#define CONTENT_WINDOW_GRAPHICS_ITEM_H

#include "ContentWindowInterface.h"
#include <QtGui>
#include <boost/shared_ptr.hpp>

class ContentWindow;

class ContentWindowGraphicsItem : public QGraphicsRectItem, public ContentWindowInterface {

    public:

        ContentWindowGraphicsItem(boost::shared_ptr<ContentWindow> contentWindow);

        // QGraphicsRectItem painting
        void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget=0);

        // re-implemented ContentWindowInterface slots
        void setCoordinates(double x, double y, double w, double h, ContentWindowInterface * source=NULL);
        void setPosition(double x, double y, ContentWindowInterface * source=NULL);
        void setSize(double w, double h, ContentWindowInterface * source=NULL);
        void setSelected(bool selected, ContentWindowInterface * source=NULL);

        // increment the Z value of this item
        void setZToFront();

    protected:

        // QGraphicsRectItem events
        void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
        void mousePressEvent(QGraphicsSceneMouseEvent * event);
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event);
        void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);

    private:

        // resizing state
        bool resizing_;

        // counter used to determine stacking order in the UI
        static qreal zCounter_;
};

#endif
