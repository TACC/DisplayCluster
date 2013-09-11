#ifndef PIXELSTREAMSEGMENT_H
#define PIXELSTREAMSEGMENT_H

#include "PixelStreamSegmentParameters.h"

#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/split_member.hpp>

#include <QByteArray>

struct PixelStreamSegment {

    // parameters; kept in a separate struct to simplify network transmission
    PixelStreamSegmentParameters parameters;

    // image data for segment
    QByteArray imageData;

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void save(Archive & ar, const unsigned int) const
        {
            ar & parameters;

            int size = imageData.size();
            ar & size;

            ar & boost::serialization::make_binary_object((void *)imageData.data(), imageData.size());
        }

        template<class Archive>
        void load(Archive & ar, const unsigned int)
        {
            ar & parameters;

            int size;
            ar & size;
            imageData.resize(size);

            ar & boost::serialization::make_binary_object((void *)imageData.data(), size);
        }

        BOOST_SERIALIZATION_SPLIT_MEMBER()
};



#endif // PIXELSTREAMSEGMENT_H
