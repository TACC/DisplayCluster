#ifndef CONTENT_GRAPHICS_ITEM_H
#define CONTENT_GRAPHICS_ITEM_H

#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/weak_ptr.hpp>

#include "Content.h"
class DisplayGroup;

class ContentWindow : public QGraphicsRectItem, public boost::enable_shared_from_this<ContentWindow> {

    public:

        ContentWindow(); // no-argument constructor required for serialization
        ContentWindow(boost::shared_ptr<Content> content);

        boost::shared_ptr<Content> getContent();

        boost::shared_ptr<DisplayGroup> getDisplayGroup();
        void setDisplayGroup(boost::shared_ptr<DisplayGroup> displayGroup);

        // window coordinates
        void setCoordinates(double x, double y, double w, double h);
        void getCoordinates(double &x, double &y, double &w, double &h);

        // panning and zooming
        void setCenterCoordinates(double centerX, double centerY);
        void getCenterCoordinates(double &centerX, double &centerY);

        void setZoom(double zoom);
        double getZoom();

        // aspect ratio correction
        void fixAspectRatio();

        // GLWindow rendering
        void render();

        // QGraphicsRectItem painting
        void paint(QPainter * painter, const QStyleOptionGraphicsItem * option, QWidget * widget=0);

        void dump()
        {
          std::cerr << "x: " << x_ << " " << "y: " << y_ << " "
                    << "w: " << w_ << " " << "w: " << h_ << ": "
                    << content_->getURI() << "\n";
        }


    protected:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & content_;
            ar & displayGroup_;
            ar & x_;
            ar & y_;
            ar & w_;
            ar & h_;
            ar & centerX_;
            ar & centerY_;
            ar & zoom_;
            ar & resizing_;
            ar & selected_;
        }

        // QGraphicsRectItem events
        void mouseMoveEvent(QGraphicsSceneMouseEvent * event);
        void mousePressEvent(QGraphicsSceneMouseEvent * event);
        void mouseDoubleClickEvent(QGraphicsSceneMouseEvent * event);
        void mouseReleaseEvent(QGraphicsSceneMouseEvent * event);
        QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    private:

        bool initialized_;

        boost::shared_ptr<Content> content_;

        boost::weak_ptr<DisplayGroup> displayGroup_;

        // window coordinates
        double x_;
        double y_;
        double w_;
        double h_;

        // panning and zooming
        double centerX_;
        double centerY_;

        double zoom_;

        // window state
        bool resizing_;
        bool selected_;

        // counter used to determine stacking order in the UI
        static qreal zCounter_;

        void getButtonDimensions(float &width, float &height);
};

typedef boost::shared_ptr<ContentWindow> pContentWindow;

class pyContentWindow
{
public:
  pyContentWindow(pyContent content) { ptr = pContentWindow(new ContentWindow(content.get())); }
  ~pyContentWindow() {}

  pContentWindow get() const {return ptr;}

private:
  pContentWindow ptr;

};


#endif
