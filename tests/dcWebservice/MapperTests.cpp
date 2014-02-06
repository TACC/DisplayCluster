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


#define BOOST_TEST_MODULE MapperTests
#include "dcWebservice/Mapper.h"
#include "dcWebservice/DefaultHandler.h"
#include <boost/test/unit_test.hpp>
namespace ut = boost::unit_test;

BOOST_AUTO_TEST_CASE( testEmptyMapper )
{
    dcWebservice::Mapper mapper;
    BOOST_CHECK( dynamic_cast<const dcWebservice::DefaultHandler*>(&mapper.getHandler("/video" )));
    BOOST_CHECK( dynamic_cast<const dcWebservice::DefaultHandler*>(&mapper.getHandler("/video/12344" )));
}

BOOST_AUTO_TEST_CASE( testUrlDoesNotMatchAnyRegex )
{
    dcWebservice::Mapper mapper;
    dcWebservice::HandlerPtr handler(new dcWebservice::DefaultHandler());
    mapper.addHandler("/video/play/(.*)", handler);
    BOOST_CHECK( dynamic_cast<const dcWebservice::DefaultHandler*>(&mapper.getHandler("/video" )));
    BOOST_CHECK( dynamic_cast<const dcWebservice::DefaultHandler*>(&mapper.getHandler("/video/12344" )));
}

BOOST_AUTO_TEST_CASE( testUrlMatchesFirstRegex )
{
    dcWebservice::Mapper mapper;
    dcWebservice::HandlerPtr handler1(new dcWebservice::DefaultHandler());
    dcWebservice::HandlerPtr handler2(new dcWebservice::DefaultHandler());
    mapper.addHandler("/video/play/(.*)", handler1);
    mapper.addHandler("/nop/", handler2);
    BOOST_CHECK_EQUAL( handler1.get(), &mapper.getHandler("/video/play/1234") );
    BOOST_CHECK_EQUAL( handler1.get(), &mapper.getHandler("/video/play/") );
    BOOST_CHECK_NE( handler2.get(), &mapper.getHandler("/video/play/1234") );
    BOOST_CHECK( dynamic_cast<const dcWebservice::DefaultHandler*>(&mapper.getHandler("/video/play" )));
}

BOOST_AUTO_TEST_CASE( testMapperReturnsFirstMatchWhenMoreThanOnePossible )
{
    dcWebservice::Mapper mapper;
    dcWebservice::HandlerPtr handler1(new dcWebservice::DefaultHandler());
    dcWebservice::HandlerPtr handler2(new dcWebservice::DefaultHandler());
    mapper.addHandler("/video/play/", handler1);
    mapper.addHandler("/video/play/(.*)", handler2);
    BOOST_CHECK_EQUAL( handler1.get(), &mapper.getHandler("/video/play/") );
    BOOST_CHECK_EQUAL( handler2.get(), &mapper.getHandler("/video/play/1234") );
    BOOST_CHECK_NE( handler1.get(), &mapper.getHandler("/video/play/1234") );
    BOOST_CHECK( dynamic_cast<const dcWebservice::DefaultHandler*>(&mapper.getHandler("/video/play" )));
}

BOOST_AUTO_TEST_CASE( testIncorrectRegex )
{
    dcWebservice::Mapper mapper;
    dcWebservice::HandlerPtr handler1(new dcWebservice::DefaultHandler());
    BOOST_CHECK_EQUAL(false, mapper.addHandler("/video/(.*", handler1));
}
