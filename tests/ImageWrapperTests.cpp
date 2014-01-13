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

#define BOOST_TEST_MODULE ImageWrapper
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

#include "dcstream/ImageWrapper.h"

BOOST_AUTO_TEST_CASE( testImageBufferSize )
{
    char* data = 0;

    {
        dc::ImageWrapper imageWrapper(data, 7, 5, dc::ARGB);
        BOOST_CHECK_EQUAL( imageWrapper.getBufferSize(), 7*5*4 );
    }
    {
        dc::ImageWrapper imageWrapper(data, 256, 512, dc::ARGB);
        BOOST_CHECK_EQUAL( imageWrapper.getBufferSize(), 256*512*4 );
    }
    {
        dc::ImageWrapper imageWrapper(data, 256, 512, dc::RGB);
        BOOST_CHECK_EQUAL( imageWrapper.getBufferSize(), 256*512*3 );
    }
}


BOOST_AUTO_TEST_CASE( testImageBytesPerPixel )
{
    char* data = 0;

    {
        dc::ImageWrapper imageWrapper(data, 256, 512, dc::RGB);
        BOOST_CHECK_EQUAL( imageWrapper.getBytesPerPixel(), 3 );
    }
    {
        dc::ImageWrapper imageWrapper(data, 256, 512, dc::RGBA);
        BOOST_CHECK_EQUAL( imageWrapper.getBytesPerPixel(), 4 );
    }
    {
        dc::ImageWrapper imageWrapper(data, 256, 512, dc::ARGB);
        BOOST_CHECK_EQUAL( imageWrapper.getBytesPerPixel(), 4 );
    }
    {
        dc::ImageWrapper imageWrapper(data, 256, 512, dc::BGR);
        BOOST_CHECK_EQUAL( imageWrapper.getBytesPerPixel(), 3 );
    }
    {
        dc::ImageWrapper imageWrapper(data, 256, 512, dc::BGRA);
        BOOST_CHECK_EQUAL( imageWrapper.getBytesPerPixel(), 4 );
    }
    {
        dc::ImageWrapper imageWrapper(data, 256, 512, dc::ABGR);
        BOOST_CHECK_EQUAL( imageWrapper.getBytesPerPixel(), 4 );
    }
}

BOOST_AUTO_TEST_CASE( testImageReorderGLImageData )
{
    {
        char dataIn[] =
        {
            1,1,1, 2,2,2, 3,3,3, 4,4,4,
            5,5,5, 6,6,6, 7,7,7, 8,8,8
        };
        char dataOut[] =
        {
            5,5,5, 6,6,6, 7,7,7, 8,8,8,
            1,1,1, 2,2,2, 3,3,3, 4,4,4
        };

        dc::ImageWrapper::swapYAxis( dataIn, 4, 2, 3 );

        BOOST_CHECK_EQUAL_COLLECTIONS( dataIn, dataIn+24, dataOut, dataOut+24 );
    }

    {
        char dataIn[] =
        {
            1,1,1, 2,2,2,
            3,3,3, 4,4,4,
            5,5,5, 6,6,6,
            7,7,7, 8,8,8
        };
        char dataOut[] =
        {
            7,7,7, 8,8,8,
            5,5,5, 6,6,6,
            3,3,3, 4,4,4,
            1,1,1, 2,2,2
        };

        dc::ImageWrapper::swapYAxis( dataIn, 2, 4, 3 );

        BOOST_CHECK_EQUAL_COLLECTIONS( dataIn, dataIn+24, dataOut, dataOut+24 );
    }

    {
        char dataIn[] =
        {
            1,1,1,2, 2,2,3,3,
            3,4,4,4, 5,5,5,6,
            6,6,7,7, 7,8,8,8
        };
        char dataOut[] =
        {
            6,6,7,7, 7,8,8,8,
            3,4,4,4, 5,5,5,6,
            1,1,1,2, 2,2,3,3
        };

        dc::ImageWrapper::swapYAxis( dataIn, 2, 3, 4 );

        BOOST_CHECK_EQUAL_COLLECTIONS( dataIn, dataIn+24, dataOut, dataOut+24 );
    }

    {
        char dataIn[] =
        {
            1,1,1,2, 2,2,3,3,
            3,4,4,4, 5,5,5,6,
            6,6,7,7, 7,8,8,8
        };
        char dataOut[] =
        {
            1,1,1,2, 2,2,3,3,
            3,4,4,4, 5,5,5,6,
            6,6,7,7, 7,8,8,8
        };

        dc::ImageWrapper::swapYAxis( dataIn, 2, 3, 4 );
        dc::ImageWrapper::swapYAxis( dataIn, 2, 3, 4 );

        BOOST_CHECK_EQUAL_COLLECTIONS( dataIn, dataIn+24, dataOut, dataOut+24 );
    }

}
