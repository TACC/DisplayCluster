#ifndef MARKER_H
#define MARKER_H

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

class Marker {

    public:

        Marker();

        void setPosition(float x, float y);
        void getPosition(float &x, float &y);

        void render();

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
};

#endif
