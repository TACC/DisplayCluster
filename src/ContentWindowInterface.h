#ifndef CONTENT_WINDOW_INTERFACE_H
#define CONTENT_WINDOW_INTERFACE_H

#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

class ContentWindowManager;

class ContentWindowInterface : public QObject {
    Q_OBJECT

    public:

        ContentWindowInterface() { }
        ContentWindowInterface(boost::shared_ptr<ContentWindowManager> contentWindowManager);

        boost::shared_ptr<ContentWindowManager> getContentWindowManager();

        void getContentDimensions(int &contentWidth, int &contentHeight);
        void getCoordinates(double &x, double &y, double &w, double &h);
        void getPosition(double &x, double &y);
        void getSize(double &w, double &h);
        void getCenter(double &centerX, double &centerY);
        double getZoom();
        bool getSelected();

        // button dimensions
        void getButtonDimensions(float &width, float &height);

        // aspect ratio correction
        void fixAspectRatio(ContentWindowInterface * source=NULL);

    public slots:

        // these methods set the local copies of the state variables if source != this
        // they will emit signals if source == NULL or if this is a ContentWindowManager object
        // the source argument should not be provided by users -- only by these functions
        virtual void setContentDimensions(int contentWidth, int contentHeight, ContentWindowInterface * source=NULL);
        virtual void setCoordinates(double x, double y, double w, double h, ContentWindowInterface * source=NULL);
        virtual void setPosition(double x, double y, ContentWindowInterface * source=NULL);
        virtual void setSize(double w, double h, ContentWindowInterface * source=NULL);
        virtual void scaleSize(double factor, ContentWindowInterface * source=NULL);
        virtual void setCenter(double centerX, double centerY, ContentWindowInterface * source=NULL);
        virtual void setZoom(double zoom, ContentWindowInterface * source=NULL);
        virtual void setSelected(bool selected, ContentWindowInterface * source=NULL);
        virtual void moveToFront(ContentWindowInterface * source=NULL);
        virtual void close(ContentWindowInterface * source=NULL);

    signals:

        // emitting these signals will trigger updates on the corresponding ContentWindowManager
        // as well as all other ContentWindowInterfaces to that ContentWindowManager
        void contentDimensionsChanged(int contentWidth, int contentHeight, ContentWindowInterface * source);
        void coordinatesChanged(double x, double y, double w, double h, ContentWindowInterface * source);
        void positionChanged(double x, double y, ContentWindowInterface * source);
        void sizeChanged(double w, double h, ContentWindowInterface * source);
        void centerChanged(double centerX, double centerY, ContentWindowInterface * source);
        void zoomChanged(double zoom, ContentWindowInterface * source);
        void selectedChanged(bool selected, ContentWindowInterface * source);
        void movedToFront(ContentWindowInterface * source);
        void closed(ContentWindowInterface * source);

    protected:

        // optional: reference to ContentWindowManager for non-ContentWindowManager objects
        boost::weak_ptr<ContentWindowManager> contentWindowManager_;

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
