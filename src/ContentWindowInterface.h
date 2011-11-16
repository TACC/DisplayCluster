#ifndef CONTENT_WINDOW_INTERFACE_H
#define CONTENT_WINDOW_INTERFACE_H

#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

class ContentWindow;

class ContentWindowInterface : public QObject {
    Q_OBJECT

    public:

        ContentWindowInterface() { }
        ContentWindowInterface(boost::shared_ptr<ContentWindow> contentWindow);

        boost::shared_ptr<ContentWindow> getContentWindow();

        void getContentDimensions(int &contentWidth, int &contentHeight);
        void getCoordinates(double &x, double &y, double &w, double &h);
        void getPosition(double &x, double &y);
        void getSize(double &w, double &h);
        void getCenter(double &centerX, double &centerY);
        double getZoom();
        bool getSelected();

        // aspect ratio correction
        void fixAspectRatio(ContentWindowInterface * source=NULL);

    public slots:

        // these methods set the local copies of the state variables if source != this
        // they will emit signals if source == NULL or if this is a ContentWindow object
        // the source argument should not be provided by users -- only by these functions
        virtual void setContentDimensions(int contentWidth, int contentHeight, ContentWindowInterface * source=NULL);
        virtual void setCoordinates(double x, double y, double w, double h, ContentWindowInterface * source=NULL);
        virtual void setPosition(double x, double y, ContentWindowInterface * source=NULL);
        virtual void setSize(double w, double h, ContentWindowInterface * source=NULL);
        virtual void setCenter(double centerX, double centerY, ContentWindowInterface * source=NULL);
        virtual void setZoom(double zoom, ContentWindowInterface * source=NULL);
        virtual void setSelected(bool selected, ContentWindowInterface * source=NULL);
        virtual void moveToFront(ContentWindowInterface * source=NULL);
        virtual void destroy(ContentWindowInterface * source=NULL);

    signals:

        // emitting these signals will trigger updates on the corresponding ContentWindow
        // as well as all other ContentWindowInterfaces to that ContentWindow
        void contentDimensionsChanged(int contentWidth, int contentHeight, ContentWindowInterface * source);
        void coordinatesChanged(double x, double y, double w, double h, ContentWindowInterface * source);
        void positionChanged(double x, double y, ContentWindowInterface * source);
        void sizeChanged(double w, double h, ContentWindowInterface * source);
        void centerChanged(double centerX, double centerY, ContentWindowInterface * source);
        void zoomChanged(double zoom, ContentWindowInterface * source);
        void selectedChanged(bool selected, ContentWindowInterface * source);
        void movedToFront(ContentWindowInterface * source);
        void destroyed(ContentWindowInterface * source);

    protected:

        // optional: reference to ContentWindow for non-ContentWindow objects
        boost::weak_ptr<ContentWindow> contentWindow_;

        // content dimensions
        int contentWidth_;
        int contentHeight_;

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
        bool selected_;
};

#endif
