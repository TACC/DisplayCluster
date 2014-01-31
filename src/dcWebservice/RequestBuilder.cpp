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

#include "RequestBuilder.h"

#include "Request.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <vector>

namespace {
const char * CONTENT_LENGTH = "CONTENT_LENGTH";
const char * REQUEST_METHOD = "REQUEST_METHOD";
const char * REQUEST_URI = "REQUEST_URI";
const char * DOCUMENT_URI = "DOCUMENT_URI";
const char * QUERY_STRING = "QUERY_STRING";
const char * AT = "&";
const char * EQ = "=";
}

namespace dcWebservice
{

RequestBuilder::~RequestBuilder() {}

RequestPtr RequestBuilder::buildRequest(FCGX_Request& fcgiRequest)
{
    RequestPtr request(new Request);

    // Get data from the request's body
    request->data = _getData(fcgiRequest);

    // Capture common headers
    request->method = std::string(FCGX_GetParam(REQUEST_METHOD, fcgiRequest.envp));
    request->url = std::string(FCGX_GetParam(REQUEST_URI, fcgiRequest.envp));
    request->queryString = std::string(FCGX_GetParam(QUERY_STRING, fcgiRequest.envp));
    request->resource = std::string(FCGX_GetParam(DOCUMENT_URI, fcgiRequest.envp));
    // Capture querystring parameters
    _populateParameters(request);

    // Capture HTTP headers
    _populateHttpHeaders(fcgiRequest, request);

    return request;
}

std::string RequestBuilder::_getData(FCGX_Request& fcgiRequest)
{
    const char * contentLength = FCGX_GetParam(CONTENT_LENGTH, fcgiRequest.envp);

    if(std::string(contentLength).empty())
        return "";

    const int length = atoi(contentLength);
    char buffer[length + 1];
    buffer[length] = '\0';
    int read = FCGX_GetStr(buffer, length, fcgiRequest.in);
    if (read != 0)
        return std::string(buffer);
    else
        return "";
}

void RequestBuilder::_populateParameters(RequestPtr fcgiRequest)
{
    if(!fcgiRequest->queryString.empty())
    {
        std::vector<std::string> pairs;
        boost::split(pairs, fcgiRequest->queryString, boost::is_any_of(AT));

        for(std::vector<std::string>::const_iterator it = pairs.begin() ;
            it != pairs.end() ; ++it)
        {
            if (it->empty())
                continue;

            std::vector<std::string> keyValue;
            boost::split(keyValue, *it, boost::is_any_of(EQ));
            if (keyValue.size() == 2 && !keyValue[0].empty())
                fcgiRequest->parameters[keyValue[0]] = keyValue[1];
            else if (keyValue.size() ==1)
                fcgiRequest->parameters[keyValue[0]] = "";
        }
    }
}

void RequestBuilder::_populateHttpHeaders(FCGX_Request& fcgiRequest,
                                                  RequestPtr request)
{
    char ** envp = fcgiRequest.envp;
    while(*envp)
    {
        std::string pair = *envp;
        if(pair.find("HTTP_",0) == 0) {
            std::vector<std::string> keyValue;
            boost::split(keyValue, pair, boost::is_any_of(EQ));
            request->httpHeaders[keyValue[0]] = keyValue[1];
        }
        envp++;
    }
}

}
