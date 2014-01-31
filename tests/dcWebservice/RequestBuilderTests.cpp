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


#define BOOST_TEST_MODULE RequestBuilderTests
#include <boost/test/unit_test.hpp>
#include "dcWebservice/RequestBuilder.h"
#include "dcWebservice/Request.h"

namespace ut = boost::unit_test;

char ** populateEnv(const std::string& queryString="")
{
    char ** envp = new char*[8];
    int i = 0;
    envp[i++] = const_cast<char *>("REQUEST_METHOD=GET");
    envp[i++] = const_cast<char *>("REQUEST_URI=/media/index.htm");
    envp[i++] = const_cast<char *>("DOCUMENT_URI=/media/index.htm");

    std::string qs = "QUERY_STRING=" + queryString;
    envp[i] = new char[qs.length() + 1];
    strcpy(envp[i++], qs.c_str());

    envp[i++] = const_cast<char *>("CONTENT_LENGTH=");
    envp[i++]= const_cast<char *>("HTTP_ACCEPT=text/html");
    envp[i++] = 0;
    return envp;
}

void freeMemory(char** envp) {
    delete [] envp[3];
    delete [] envp;
}


void checkEmptyQueryString(const std::string& qs)
{
    char ** envp = populateEnv(qs);
    FCGX_Init();
    FCGX_Request fcgiRequest;

    fcgiRequest.envp = envp;
    dcWebservice::RequestBuilder builder;
    dcWebservice::RequestPtr request = builder.buildRequest(fcgiRequest);
    BOOST_CHECK_EQUAL(request->queryString, qs);
    BOOST_CHECK_EQUAL(0, request->parameters.size());
    freeMemory(envp);
}


BOOST_AUTO_TEST_CASE( testRequestWithoutData )
{
    std::string qs = "key1=val1&key2=val2&key3&key4=";
    char ** envp = populateEnv(qs);
    FCGX_Request fcgiRequest;
    fcgiRequest.envp = envp;

    dcWebservice::RequestBuilder builder;
    dcWebservice::RequestPtr request = builder.buildRequest(fcgiRequest);

    BOOST_CHECK_EQUAL(request->method, "GET");
    BOOST_CHECK_EQUAL(request->url, "/media/index.htm");
    BOOST_CHECK_EQUAL(request->resource, "/media/index.htm");
    BOOST_CHECK_EQUAL(request->queryString, qs);
    BOOST_CHECK_EQUAL(request->httpHeaders["HTTP_ACCEPT"], "text/html");
    BOOST_CHECK_EQUAL(4, request->parameters.size());
    BOOST_CHECK_EQUAL(request->parameters["key1"], "val1");
    BOOST_CHECK_EQUAL(request->parameters["key2"], "val2");
    BOOST_CHECK_EQUAL(request->parameters["key3"], "");
    BOOST_CHECK_EQUAL(request->parameters["key4"], "");
    freeMemory(envp);
}

BOOST_AUTO_TEST_CASE( testRequestEmptyQueryString )
{
    checkEmptyQueryString("");
    checkEmptyQueryString("&");
    checkEmptyQueryString("&&&");
    checkEmptyQueryString("=");
    checkEmptyQueryString("=&=");
    checkEmptyQueryString("=a&=b&&");
}
