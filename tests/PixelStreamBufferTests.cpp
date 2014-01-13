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

#define BOOST_TEST_MODULE PixelStreamBufferTests
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include "PixelStreamBuffer.h"
#include "PixelStreamSegment.h"

BOOST_AUTO_TEST_CASE( TestAddAndRemoveSources )
{
    PixelStreamBuffer buffer;

    BOOST_REQUIRE_EQUAL( buffer.getSourceCount(), 0 );

    buffer.addSource(53);
    BOOST_CHECK_EQUAL( buffer.getSourceCount(), 1 );

    buffer.addSource(11981);
    BOOST_CHECK_EQUAL( buffer.getSourceCount(), 2 );

    buffer.addSource(888);
    buffer.removeSource(53);
    BOOST_CHECK_EQUAL( buffer.getSourceCount(), 2 );

    buffer.removeSource(888);
    buffer.removeSource(11981);
    BOOST_CHECK_EQUAL( buffer.getSourceCount(), 0 );
}


BOOST_AUTO_TEST_CASE( TestCompleteAFrame )
{
    const size_t sourceIndex = 46;

    PixelStreamBuffer buffer;
    buffer.addSource(sourceIndex);

    dc::PixelStreamSegment segment;
    segment.parameters.x = 0;
    segment.parameters.y = 0;
    segment.parameters.width = 128;
    segment.parameters.height = 256;

    buffer.insertSegment(segment, sourceIndex);
    BOOST_CHECK( !buffer.hasFrameComplete() );

    buffer.finishFrameForSource(sourceIndex);
    BOOST_CHECK( buffer.hasFrameComplete() );

    QSize frameSize = buffer.getFrameSize();
    BOOST_CHECK( buffer.hasFrameComplete() );
    BOOST_CHECK( buffer.isFirstFrame() );
    BOOST_CHECK_EQUAL( frameSize.width(), segment.parameters.width );
    BOOST_CHECK_EQUAL( frameSize.height(), segment.parameters.height );

    PixelStreamSegments segments = buffer.getFrame();

    BOOST_CHECK_EQUAL( segments.size(), 1 );
    BOOST_CHECK( !buffer.hasFrameComplete() );
    BOOST_CHECK( !buffer.isFirstFrame() );
}


PixelStreamSegments generateTestSegments()
{
    PixelStreamSegments segments;

    dc::PixelStreamSegment segment1;
    segment1.parameters.x = 0;
    segment1.parameters.y = 0;
    segment1.parameters.width = 128;
    segment1.parameters.height = 256;

    dc::PixelStreamSegment segment2;
    segment2.parameters.x = 128;
    segment2.parameters.y = 0;
    segment2.parameters.width = 64;
    segment2.parameters.height = 256;

    dc::PixelStreamSegment segment3;
    segment3.parameters.x = 0;
    segment3.parameters.y = 256;
    segment3.parameters.width = 128;
    segment3.parameters.height = 512;

    dc::PixelStreamSegment segment4;
    segment4.parameters.x = 128;
    segment4.parameters.y = 256;
    segment4.parameters.width = 64;
    segment4.parameters.height = 512;

    segments.push_back(segment1);
    segments.push_back(segment2);
    segments.push_back(segment3);
    segments.push_back(segment4);

    return segments;
}

BOOST_AUTO_TEST_CASE( TestCompleteACompositeFrameSingleSource )
{
    const size_t sourceIndex = 46;

    PixelStreamBuffer buffer;
    buffer.addSource(sourceIndex);

    PixelStreamSegments testSegments = generateTestSegments();

    buffer.insertSegment(testSegments[0], sourceIndex);
    buffer.insertSegment(testSegments[1], sourceIndex);
    buffer.insertSegment(testSegments[2], sourceIndex);
    buffer.insertSegment(testSegments[3], sourceIndex);
    BOOST_CHECK( !buffer.hasFrameComplete() );

    buffer.finishFrameForSource(sourceIndex);
    BOOST_CHECK( buffer.hasFrameComplete() );

    QSize frameSize = buffer.getFrameSize();
    BOOST_CHECK( buffer.hasFrameComplete() );
    BOOST_CHECK( buffer.isFirstFrame() );
    BOOST_CHECK_EQUAL( frameSize.width(), 192 );
    BOOST_CHECK_EQUAL( frameSize.height(), 768 );

    PixelStreamSegments segments = buffer.getFrame();
    frameSize = buffer.getFrameSize();
    BOOST_CHECK_EQUAL( frameSize.width(), 0 );
    BOOST_CHECK_EQUAL( frameSize.height(), 0 );


    BOOST_CHECK_EQUAL( segments.size(), 4 );
    BOOST_CHECK( !buffer.hasFrameComplete() );
    BOOST_CHECK( !buffer.isFirstFrame() );
}



BOOST_AUTO_TEST_CASE( TestCompleteACompositeFrameMultipleSources )
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;
    const size_t sourceIndex3 = 11;

    PixelStreamBuffer buffer;
    buffer.addSource(sourceIndex1);
    buffer.addSource(sourceIndex2);
    buffer.addSource(sourceIndex3);

    PixelStreamSegments testSegments = generateTestSegments();

    buffer.insertSegment(testSegments[0], sourceIndex1);
    buffer.insertSegment(testSegments[1], sourceIndex2);
    buffer.insertSegment(testSegments[2], sourceIndex3);
    BOOST_CHECK( !buffer.hasFrameComplete() );

    buffer.finishFrameForSource(sourceIndex1);
    BOOST_CHECK( !buffer.hasFrameComplete() );

    buffer.finishFrameForSource(sourceIndex2);
    BOOST_CHECK( !buffer.hasFrameComplete() );

    buffer.insertSegment(testSegments[3], sourceIndex3);
    buffer.finishFrameForSource(sourceIndex3);
    BOOST_CHECK( buffer.hasFrameComplete() );

    BOOST_CHECK( buffer.hasFrameComplete() );
    BOOST_CHECK( buffer.isFirstFrame() );
    QSize frameSize = buffer.getFrameSize();
    BOOST_CHECK_EQUAL( frameSize.width(), 192 );
    BOOST_CHECK_EQUAL( frameSize.height(), 768 );

    PixelStreamSegments segments = buffer.getFrame();

    BOOST_CHECK_EQUAL( segments.size(), 4 );
    BOOST_CHECK( !buffer.hasFrameComplete() );
    BOOST_CHECK( !buffer.isFirstFrame() );
}


BOOST_AUTO_TEST_CASE( TestRemoveSourceWhileStreaming )
{
    const size_t sourceIndex1 = 46;
    const size_t sourceIndex2 = 819;

    PixelStreamBuffer buffer;
    buffer.addSource(sourceIndex1);
    buffer.addSource(sourceIndex2);

    PixelStreamSegments testSegments = generateTestSegments();

    // First Frame - 2 sources
    buffer.insertSegment(testSegments[0], sourceIndex1);
    buffer.insertSegment(testSegments[1], sourceIndex1);
    buffer.insertSegment(testSegments[2], sourceIndex2);
    buffer.insertSegment(testSegments[3], sourceIndex2);
    BOOST_CHECK( !buffer.hasFrameComplete() );
    buffer.finishFrameForSource(sourceIndex1);
    BOOST_CHECK( !buffer.hasFrameComplete() );
    buffer.finishFrameForSource(sourceIndex2);
    BOOST_CHECK( buffer.hasFrameComplete() );

    PixelStreamSegments segments = buffer.getFrame();

    BOOST_CHECK_EQUAL( segments.size(), 4 );
    BOOST_CHECK( !buffer.hasFrameComplete() );
    BOOST_CHECK( !buffer.isFirstFrame() );

    // Second frame - 1 source
    buffer.removeSource(sourceIndex2);

    buffer.insertSegment(testSegments[0], sourceIndex1);
    buffer.insertSegment(testSegments[1], sourceIndex1);
    BOOST_CHECK( !buffer.hasFrameComplete() );
    buffer.finishFrameForSource(sourceIndex1);
    BOOST_CHECK( buffer.hasFrameComplete() );

    QSize frameSize = buffer.getFrameSize();
    BOOST_CHECK_EQUAL( frameSize.width(), 192 );
    BOOST_CHECK_EQUAL( frameSize.height(), 256 );

    segments = buffer.getFrame();
    BOOST_CHECK_EQUAL( segments.size(), 2 );
    BOOST_CHECK( !buffer.hasFrameComplete() );
    BOOST_CHECK( !buffer.isFirstFrame() );

}
