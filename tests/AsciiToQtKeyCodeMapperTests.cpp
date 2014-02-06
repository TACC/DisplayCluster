/*********************************************************************/
/* Copyright (c) 2014, EPFL/Blue Brain Project                       */
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

#include "ws/AsciiToQtKeyCodeMapper.h"

#include <qnamespace.h>

BOOST_AUTO_TEST_CASE( TestSpecialKeyCode )
{
    AsciiToQtKeyCodeMapper mapper;
    BOOST_CHECK_EQUAL(mapper.getQtKeyCode(8), Qt::Key_Backspace);
    BOOST_CHECK_EQUAL(mapper.getQtKeyCode(9), Qt::Key_Tab);
    BOOST_CHECK_EQUAL(mapper.getQtKeyCode(10), Qt::Key_Return);
    BOOST_CHECK_EQUAL(mapper.getQtKeyCode(13), Qt::Key_Enter);
    BOOST_CHECK_EQUAL(mapper.getQtKeyCode(27), Qt::Key_Escape);
}

BOOST_AUTO_TEST_CASE( TestNormalCharater )
{
    AsciiToQtKeyCodeMapper mapper;
    BOOST_CHECK_EQUAL(mapper.getQtKeyCode('a'), 'a');
    BOOST_CHECK_EQUAL(mapper.getQtKeyCode('b'), 'b');
    BOOST_CHECK_EQUAL(mapper.getQtKeyCode('c'), 'c');
    BOOST_CHECK_EQUAL(mapper.getQtKeyCode('A'), 'A');
    BOOST_CHECK_EQUAL(mapper.getQtKeyCode('B'), 'B');
}
