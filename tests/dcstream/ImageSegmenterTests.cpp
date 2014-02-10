/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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

#define BOOST_TEST_MODULE ImageSegmenterTests
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include "dcstream/ImageWrapper.h"
#include "dcstream/ImageSegmenter.h"
#include "PixelStreamSegment.h"

BOOST_AUTO_TEST_CASE( testImageSegmenterSegmentParameters )
{
    char data[] =
    {
        1,1,1, 2,2,2, 3,3,3, 4,4,4,
        5,5,5, 6,6,6, 7,7,7, 8,8,8,
        1,1,1, 2,2,2, 3,3,3, 4,4,4,
        5,5,5, 6,6,6, 7,7,7, 8,8,8,
        1,1,1, 2,2,2, 3,3,3, 4,4,4,
        5,5,5, 6,6,6, 7,7,7, 8,8,8,
        1,1,1, 2,2,2, 3,3,3, 4,4,4,
        5,5,5, 6,6,6, 7,7,7, 8,8,8
    };
    dc::ImageWrapper imageWrapper(data, 4, 8, dc::RGB);

    {
        dc::ImageSegmenter segmenter;

        dc::PixelStreamSegments segments = segmenter.generateSegments(imageWrapper);
        BOOST_REQUIRE_EQUAL( segments.size(), 1 );

        dc::PixelStreamSegment& segment = segments.front();
        BOOST_CHECK_EQUAL( segment.parameters.x, 0 );
        BOOST_CHECK_EQUAL( segment.parameters.y, 0 );
        BOOST_CHECK_EQUAL( segment.parameters.width, 4 );
        BOOST_CHECK_EQUAL( segment.parameters.height, 8 );
    }

    {
        dc::ImageSegmenter segmenter;
        segmenter.setNominalSegmentDimensions(2,2);

        dc::PixelStreamSegments segments = segmenter.generateSegments(imageWrapper);
        BOOST_REQUIRE_EQUAL( segments.size(), 8 );

        unsigned int i = 0;
        for(dc::PixelStreamSegments::const_iterator it = segments.begin(); it != segments.end(); ++it, ++i)
        {
            const dc::PixelStreamSegment& segment = *it;
            BOOST_CHECK_EQUAL( segment.parameters.x, i%4 );
            BOOST_CHECK_EQUAL( segment.parameters.y, 2*(i/4) );
            BOOST_CHECK_EQUAL( segment.parameters.width, 2 );
            BOOST_CHECK_EQUAL( segment.parameters.height, 2 );
            ++i;
        }
    }

    {
        dc::ImageSegmenter segmenter;
        segmenter.setNominalSegmentDimensions(3,5);

        dc::PixelStreamSegments segments = segmenter.generateSegments(imageWrapper);
        BOOST_REQUIRE_EQUAL( segments.size(), 4 );

        dc::PixelStreamSegment& segment = segments[0];
        BOOST_CHECK_EQUAL( segment.parameters.x, 0 );
        BOOST_CHECK_EQUAL( segment.parameters.y, 0 );
        BOOST_CHECK_EQUAL( segment.parameters.width, 3 );
        BOOST_CHECK_EQUAL( segment.parameters.height, 5 );

        segment = segments[1];
        BOOST_CHECK_EQUAL( segment.parameters.x, 3 );
        BOOST_CHECK_EQUAL( segment.parameters.y, 0 );
        BOOST_CHECK_EQUAL( segment.parameters.width, 1 );
        BOOST_CHECK_EQUAL( segment.parameters.height, 5 );

        segment = segments[2];
        BOOST_CHECK_EQUAL( segment.parameters.x, 0 );
        BOOST_CHECK_EQUAL( segment.parameters.y, 5 );
        BOOST_CHECK_EQUAL( segment.parameters.width, 3 );
        BOOST_CHECK_EQUAL( segment.parameters.height, 3 );

        segment = segments[3];
        BOOST_CHECK_EQUAL( segment.parameters.x, 3 );
        BOOST_CHECK_EQUAL( segment.parameters.y, 5 );
        BOOST_CHECK_EQUAL( segment.parameters.width, 1 );
        BOOST_CHECK_EQUAL( segment.parameters.height, 3 );
    }
}


BOOST_AUTO_TEST_CASE( testImageSegmenterSingleSegmentData )
{
    char dataIn[] =
    {
        1,1,1, 2,2,2, 3,3,3, 4,4,4,
        5,5,5, 6,6,6, 7,7,7, 8,8,8,
        1,1,1, 2,2,2, 3,3,3, 4,4,4,
        5,5,5, 6,6,6, 7,7,7, 8,8,8,
        1,1,1, 2,2,2, 3,3,3, 4,4,4,
        5,5,5, 6,6,6, 7,7,7, 8,8,8,
        1,1,1, 2,2,2, 3,3,3, 4,4,4,
        5,5,5, 6,6,6, 7,7,7, 8,8,8
    };
    dc::ImageWrapper imageWrapper(dataIn, 4, 8, dc::RGB);
    imageWrapper.compressionPolicy = dc::COMPRESSION_OFF;

    dc::ImageSegmenter segmenter;

    dc::PixelStreamSegments segments;
    segments = segmenter.generateSegments(imageWrapper);
    BOOST_REQUIRE_EQUAL( segments.size(), 1 );

    dc::PixelStreamSegment& segment = segments.front();
    const char* dataOut = segment.imageData.constData();
    BOOST_CHECK_EQUAL_COLLECTIONS( dataIn, dataIn+imageWrapper.getBufferSize(),
                                   dataOut, dataOut+imageWrapper.getBufferSize() );
}


BOOST_AUTO_TEST_CASE( testImageSegmenterUniformSegmentationData )
{
    char dataIn[] =
    {
        1,1,1, 2,2,2, 3,3,3, 4,4,4,
        5,5,5, 6,6,6, 7,7,7, 8,8,8,
        1,1,1, 2,2,2, 3,3,3, 4,4,4,
        5,5,5, 6,6,6, 7,7,7, 8,8,8,

        5,5,5, 6,6,6, 7,7,7, 8,8,8,
        1,1,1, 2,2,2, 3,3,3, 4,4,4,
        5,5,5, 6,6,6, 7,7,7, 8,8,8,
        1,1,1, 2,2,2, 3,3,3, 4,4,4
    };

    char dataSegmented[4][24] =
    {
        {
        1,1,1, 2,2,2,
        5,5,5, 6,6,6,
        1,1,1, 2,2,2,
        5,5,5, 6,6,6
        },
        {
        3,3,3, 4,4,4,
        7,7,7, 8,8,8,
        3,3,3, 4,4,4,
        7,7,7, 8,8,8
        },
        {
        5,5,5, 6,6,6,
        1,1,1, 2,2,2,
        5,5,5, 6,6,6,
        1,1,1, 2,2,2
        },
        {
        7,7,7, 8,8,8,
        3,3,3, 4,4,4,
        7,7,7, 8,8,8,
        3,3,3, 4,4,4
        }
    };

    dc::ImageWrapper imageWrapper(dataIn, 4, 8, dc::RGB);
    imageWrapper.compressionPolicy = dc::COMPRESSION_OFF;

    dc::ImageSegmenter segmenter;
    segmenter.setNominalSegmentDimensions(2,4);

    dc::PixelStreamSegments segments;
    segments = segmenter.generateSegments(imageWrapper);
    BOOST_REQUIRE_EQUAL( segments.size(), 4 );

    size_t i = 0;
    for(dc::PixelStreamSegments::const_iterator it = segments.begin(); it != segments.end(); ++it, ++i)
    {
        const dc::PixelStreamSegment& segment = *it;
        const char* dataOut = segment.imageData.constData();
        BOOST_CHECK_EQUAL_COLLECTIONS( dataSegmented[i], dataSegmented[i]+24,
                                       dataOut, dataOut+segment.imageData.size() );
    }
}


BOOST_AUTO_TEST_CASE( testImageSegmenterNonUniformSegmentationData )
{
    char dataIn[] =
    {
        1,1,1, 2,2,2, 3,3,3,   4,4,4,
        5,5,5, 6,6,6, 7,7,7,   8,8,8,
        1,1,1, 2,2,2, 3,3,3,   4,4,4,
        5,5,5, 6,6,6, 7,7,7,   8,8,8,
        5,5,5, 6,6,6, 7,7,7,   8,8,8,

        1,1,1, 2,2,2, 3,3,3,   4,4,4,
        5,5,5, 6,6,6, 7,7,7,   8,8,8,
        1,1,1, 2,2,2, 3,3,3,   4,4,4
    };

    char dataSegmented0[] =
    {
        1,1,1, 2,2,2, 3,3,3,
        5,5,5, 6,6,6, 7,7,7,
        1,1,1, 2,2,2, 3,3,3,
        5,5,5, 6,6,6, 7,7,7,
        5,5,5, 6,6,6, 7,7,7
    };
    char dataSegmented1[] =
    {
        4,4,4,
        8,8,8,
        4,4,4,
        8,8,8,
        8,8,8
    };
    char dataSegmented2[] =
    {
        1,1,1, 2,2,2, 3,3,3,
        5,5,5, 6,6,6, 7,7,7,
        1,1,1, 2,2,2, 3,3,3
    };
    char dataSegmented3[] =
    {
        4,4,4,
        8,8,8,
        4,4,4
    };
    char* dataSegmented[4];
    dataSegmented[0] = dataSegmented0;
    dataSegmented[1] = dataSegmented1;
    dataSegmented[2] = dataSegmented2;
    dataSegmented[3] = dataSegmented3;


    dc::ImageWrapper imageWrapper(dataIn, 4, 8, dc::RGB);
    imageWrapper.compressionPolicy = dc::COMPRESSION_OFF;

    dc::ImageSegmenter segmenter;
    segmenter.setNominalSegmentDimensions(3,5);

    dc::PixelStreamSegments segments;
    segments = segmenter.generateSegments(imageWrapper);
    BOOST_REQUIRE_EQUAL( segments.size(), 4 );

    size_t i = 0;
    for(dc::PixelStreamSegments::const_iterator it = segments.begin(); it != segments.end(); ++it, ++i)
    {
        const dc::PixelStreamSegment& segment = *it;
        const char* dataOut = segment.imageData.constData();
        BOOST_CHECK_EQUAL_COLLECTIONS( dataSegmented[i], dataSegmented[i]+segment.imageData.size(),
                                       dataOut, dataOut+segment.imageData.size() );
    }
}

