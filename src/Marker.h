#ifndef MARKER_H
#define MARKER_H

// todo: we need to bundle this with the application or find a better way of handling the path
#define MARKER_IMAGE_FILENAME "./data/marker.png"

// this is a fraction of the tiled display width of 1
#define MARKER_WIDTH 0.0025

#include <QtGui>
#include <QGLWidget>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

class Marker : public QObject {
    Q_OBJECT

    public:

        Marker();

        void setPosition(float x, float y);
        void getPosition(float &x, float &y);

        void render();

    signals:
        void positionChanged();

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & x_;
            ar & y_;
        }

        float x_;
        float y_;

        static GLuint textureId_;
};

#endif
