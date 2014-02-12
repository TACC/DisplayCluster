/*********************************************************************/
/* Copyright (c) 2014, EPFL/Blue Brain Project                       */
/*                     Julio Delgado <julio.delgadomangas@epfl.ch>   */
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


#define BOOST_TEST_MODULE ResponseTests
#include <boost/test/unit_test.hpp>
#include "dcWebservice/Response.h"
namespace ut = boost::unit_test;

BOOST_AUTO_TEST_CASE( testSerializeWithEmptyBody )
{
    const std::string expected = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n"
	                   "Status: 200 OK\r\n\r\n";
    std::stringstream ss;

    dcWebservice::Response response;
    response.statusCode = 200;
    response.statusMsg = "OK";
    response.body = "";

    ss << response;
    BOOST_CHECK_EQUAL(expected, response.serialize());
    BOOST_CHECK_EQUAL(ss.str(), response.serialize());
}

BOOST_AUTO_TEST_CASE( testSerializeWithNonEmptyBody )
{
    const std::string expected = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n"
	                   "Status: 200 OK\r\n\r\n{}";
    std::stringstream ss;
    dcWebservice::Response response;
    response.statusCode = 200;
    response.statusMsg = "OK";
    response.body = "{}";

    ss << response;
    BOOST_CHECK_EQUAL(expected, response.serialize());
    BOOST_CHECK_EQUAL(ss.str(), response.serialize());
}

BOOST_AUTO_TEST_CASE( testSerializeNoBodyAndCustomHeaders )
{
    const std::string expected = "HTTP/1.1 200 OK\r\nCustom-h1: 1\r\nCustom-h2: 2\r\n"
	                   "Content-Length: 0\r\nStatus: 200 OK\r\n\r\n";
    std::stringstream ss;

    dcWebservice::Response response;
    response.statusCode = 200;
    response.statusMsg = "OK";
    response.httpHeaders["Custom-h1"] = "1";
    response.httpHeaders["Custom-h2"] = "2";
    response.body = "";
    ss << response;

    BOOST_CHECK_EQUAL(expected, response.serialize());
    BOOST_CHECK_EQUAL(ss.str(), response.serialize());
}

BOOST_AUTO_TEST_CASE( test200Response )
{
    dcWebservice::Response r = *dcWebservice::Response::OK();
    BOOST_CHECK_EQUAL(200, r.statusCode);
    BOOST_CHECK_EQUAL("OK", r.statusMsg);
    BOOST_CHECK_EQUAL(0, r.httpHeaders.size());
    BOOST_CHECK_EQUAL("{\"code\":\"200\", \"msg\":\"OK\"}", r.body);
    
}

BOOST_AUTO_TEST_CASE( test404Response )
{
    dcWebservice::Response r = *dcWebservice::Response::NotFound();
    BOOST_CHECK_EQUAL(404, r.statusCode);
    BOOST_CHECK_EQUAL("Not Found", r.statusMsg);
    BOOST_CHECK_EQUAL(0, r.httpHeaders.size());
    BOOST_CHECK_EQUAL("{\"code\":\"404\", \"msg\":\"Not Found\"}", r.body);
    
}

BOOST_AUTO_TEST_CASE( test500Response )
{
    dcWebservice::Response r = *dcWebservice::Response::ServerError();
    BOOST_CHECK_EQUAL(500, r.statusCode);
    BOOST_CHECK_EQUAL("Internal Server Error", r.statusMsg);
    BOOST_CHECK_EQUAL(0, r.httpHeaders.size());
    BOOST_CHECK_EQUAL("{\"code\":\"500\", \"msg\":\"Internal Server Error\"}", r.body);
}
