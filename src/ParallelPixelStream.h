/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
/* All rights reserved.                                              */
/*                                                                   */
/* Redistribution and use in source and binary forms, with or        */
/* without modification, are permitted provided that the following   */
/* conditions are met:                                               */
/*                                                                   */
/*   1. Redistributions of source code must retain the above         */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer.                                                  */
/*                                                                   */
/*   2. Redistributions in binary form must reproduce the above      */
/*      copyright notice, this list of conditions and the following  */
/*      disclaimer in the documentation and/or other materials       */
/*      provided with the distribution.                              */
/*                                                                   */
/*    THIS  SOFTWARE IS PROVIDED  BY THE  UNIVERSITY OF  TEXAS AT    */
/*    AUSTIN  ``AS IS''  AND ANY  EXPRESS OR  IMPLIED WARRANTIES,    */
/*    INCLUDING, BUT  NOT LIMITED  TO, THE IMPLIED  WARRANTIES OF    */
/*    MERCHANTABILITY  AND FITNESS FOR  A PARTICULAR  PURPOSE ARE    */
/*    DISCLAIMED.  IN  NO EVENT SHALL THE UNIVERSITY  OF TEXAS AT    */
/*    AUSTIN OR CONTRIBUTORS BE  LIABLE FOR ANY DIRECT, INDIRECT,    */
/*    INCIDENTAL,  SPECIAL, EXEMPLARY,  OR  CONSEQUENTIAL DAMAGES    */
/*    (INCLUDING, BUT  NOT LIMITED TO,  PROCUREMENT OF SUBSTITUTE    */
/*    GOODS  OR  SERVICES; LOSS  OF  USE,  DATA,  OR PROFITS;  OR    */
/*    BUSINESS INTERRUPTION) HOWEVER CAUSED  AND ON ANY THEORY OF    */
/*    LIABILITY, WHETHER  IN CONTRACT, STRICT  LIABILITY, OR TORT    */
/*    (INCLUDING NEGLIGENCE OR OTHERWISE)  ARISING IN ANY WAY OUT    */
/*    OF  THE  USE OF  THIS  SOFTWARE,  EVEN  IF ADVISED  OF  THE    */
/*    POSSIBILITY OF SUCH DAMAGE.                                    */
/*                                                                   */
/* The views and conclusions contained in the software and           */
/* documentation are those of the authors and should not be          */
/* interpreted as representing official policies, either expressed   */
/* or implied, of The University of Texas at Austin.                 */
/*********************************************************************/

#ifndef PARALLEL_PIXEL_STREAM_H
#define PARALLEL_PIXEL_STREAM_H

#include "ParallelPixelStreamSegmentParameters.h"
#include "FactoryObject.h"
#include "PixelStream.h"
#include "Factory.hpp"
#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/split_member.hpp>
#include <string>
#include <map>
#include <vector>

// define serialize method separately from ParallelPixelStreamSegmentParameters definition
// so other (external) code can more easily include that header
namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive & ar, ParallelPixelStreamSegmentParameters & p, const unsigned int)
{
    ar & p.sourceIndex;
    ar & p.x;
    ar & p.y;
    ar & p.width;
    ar & p.height;
    ar & p.totalWidth;
    ar & p.totalHeight;
}

} // namespace serialization
} // namespace boost

struct ParallelPixelStreamSegment {

    // parameters; kept in a separate struct to simplify network transmission
    ParallelPixelStreamSegmentParameters parameters;

    // image data for segment
    QByteArray imageData;

    // pointer for pixel stream (may not be used)
    // does not get serialized
    boost::shared_ptr<PixelStream> pixelStream;

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

class ParallelPixelStream : public FactoryObject {

    public:

        ParallelPixelStream(std::string uri);

        void getDimensions(int &width, int &height);
        void render(float tX, float tY, float tW, float tH);

        void insertSegment(ParallelPixelStreamSegment segment);

        // retrieve latest segments and remove them (and older segments) from the map
        std::vector<ParallelPixelStreamSegment> getAndPopLatestSegments();

    private:

        // parallel pixel stream identifier
        std::string uri_;

        // dimensions of entire parallel pixel stream
        int width_;
        int height_;

        // segments mutex
        QMutex segmentsMutex_;

        // for each source, vector of pixel stream segments
        std::map<int, std::vector<ParallelPixelStreamSegment> > segments_;
};

// global parallel pixel stream source factory
// uses the same class, but only as a source, not for rendering
extern Factory<ParallelPixelStream> g_parallelPixelStreamSourceFactory;

#endif
