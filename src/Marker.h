#ifndef MARKER_H
#define MARKER_H

#define MARKER_IMAGE_FILENAME "marker.png"

// this is a fraction of the tiled display width of 1
#define MARKER_WIDTH 0.0025

// number of seconds before a marker stops being rendered
#define MARKER_TIMEOUT_SECONDS 5

#include <QtGui>
#include <QGLWidget>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>

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
        void serialize(Archive & ar, const unsigned int)
        {
            ar & x_;
            ar & y_;
            ar & updatedTimestamp_;
        }

        float x_;
        float y_;
        boost::posix_time::ptime updatedTimestamp_;

        static GLuint textureId_;
};

#endif
