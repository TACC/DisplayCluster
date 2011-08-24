#ifndef CONTENT_H
#define CONTENT_H

#include <string>
#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

class ContentGraphicsItem;

class Content : public boost::enable_shared_from_this<Content> {

    public:

        Content(std::string uri = "");

        std::string getURI();

        void setCoordinates(double x, double y, double w, double h);
        void getCoordinates(double &x, double &y, double &w, double &h);

        boost::shared_ptr<ContentGraphicsItem> getGraphicsItem();

        void render();

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & uri_;
            ar & x_;
            ar & y_;
            ar & w_;
            ar & h_;
        }

        std::string uri_;

        double x_;
        double y_;
        double w_;
        double h_;

        // used for GUI display
        boost::shared_ptr<ContentGraphicsItem> graphicsItem_;
};

#endif
