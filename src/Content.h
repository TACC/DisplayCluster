#ifndef CONTENT_H
#define CONTENT_H

#include <string>
#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/assume_abstract.hpp>

class DisplayGroup;
class ContentGraphicsItem;

class Content : public boost::enable_shared_from_this<Content> {

    public:

        Content(std::string uri = "");

        boost::shared_ptr<DisplayGroup> getDisplayGroup();
        void setDisplayGroup(boost::shared_ptr<DisplayGroup> displayGroup);

        std::string getURI();

        void setCoordinates(double x, double y, double w, double h, bool setGraphicsItem=true);
        void getCoordinates(double &x, double &y, double &w, double &h);

        void setCenterCoordinates(double centerX, double centerY);
        void getCenterCoordinates(double &centerX, double &centerY);

        void setZoom(double zoom);
        double getZoom();

        boost::shared_ptr<ContentGraphicsItem> getGraphicsItem();

        void render();

        // virtual method for implementing actions on advancing to a new frame
        // useful when a process has multiple GLWindows
        virtual void advance() { }

        // get a Content object of the appropriate derived type based on the URI given
        static boost::shared_ptr<Content> getContent(std::string uri);

    protected:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            // todo: do we serialize the display group? it's not necessary for MPI synchronization, but it would be for state loading...

            ar & uri_;
            ar & x_;
            ar & y_;
            ar & w_;
            ar & h_;
            ar & centerX_;
            ar & centerY_;
            ar & zoom_;
        }

        boost::weak_ptr<DisplayGroup> displayGroup_;

        std::string uri_;

        // window coordinates
        double x_;
        double y_;
        double w_;
        double h_;

        // pan and zoom parameters
        double centerX_;
        double centerY_;

        double zoom_;

        // used for GUI display
        boost::shared_ptr<ContentGraphicsItem> graphicsItem_;

        virtual void renderFactoryObject(float tX, float tY, float tW, float tH) = 0;
};

BOOST_SERIALIZATION_ASSUME_ABSTRACT(Content)

#endif
