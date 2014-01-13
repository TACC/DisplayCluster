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

#define BOOST_TEST_MODULE PixelStreamSegmentDecoderTests
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include "dcstream/ImageWrapper.h"
#include "dcstream/ImageJpegCompressor.h"
#include "ImageJpegDecompressor.h"

BOOST_AUTO_TEST_CASE( testImageCompressionAndDecompression )
{
    // Vector of RGBA data
    std::vector<char> data;
    data.reserve(8*8*4);
    for (size_t i = 0; i<8*8; ++i)
    {
        data.push_back(192); // R
        data.push_back(128); // G
        data.push_back(64);  // B
        data.push_back(255); // A
    }
    dc::ImageWrapper imageWrapper(data.data(), 8, 8, dc::RGBA);

    // Compress image
    dc::ImageJpegCompressor compressor;
    QByteArray jpegData = compressor.computeJpeg(imageWrapper, QRect(0,0,8,8));

    BOOST_REQUIRE( jpegData.size() > 0 );
    BOOST_REQUIRE( jpegData.size() != (int)data.size() );

    // Decompress image
    ImageJpegDecompressor decompressor;
    QByteArray decodedData = decompressor.decompress(jpegData);

    // Check decoded image in format RGBA
    BOOST_REQUIRE( !decodedData.isEmpty() );
    BOOST_REQUIRE_EQUAL( decodedData.size(), data.size() );

    const char* dataOut = decodedData.constData();
    BOOST_CHECK_EQUAL_COLLECTIONS( data.data(), data.data()+data.size(),
                                   dataOut, dataOut+data.size() );
}

