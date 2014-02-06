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
#ifndef REQUEST_BUILDER_H
#define REQUEST_BUILDER_H

#include <fcgiapp.h>

#include "types.h"

namespace dcWebservice
{

/**
 * This class encapsulates the logic necessary to extract information
 * from a FastCGI request (FCGX_Request), into a dcWebservice::Request object
 * that can be consumed by the rest of the application.
 */
class RequestBuilder
{
public:
    /**
     * Destructor
     */
    virtual ~RequestBuilder();

    /**
     * Creates a new dcWebservice::Request object using the information contained
     * in the FastCGI request.
     *
     * @param fcgiRequest A populated FastCGI request object.
     * @returns A boost::shared_ptr to a dcWebservice::Request object.
     */
    virtual RequestPtr buildRequest(FCGX_Request& fcgiRequest);

private:
    /*
     * Retrieves the data from the body of the FCGI request.
     * @param fcgiRequest A populated FastCGI request object.
     * @returns A string with the data found int the body or an empty string.
     */
    std::string _getData(FCGX_Request& fcgiRequest);

    /*
     * This method must be called once a query string has been added to the
     * request. The query string is parsed and the pairs key/value are added
     * to the parameters map in the request.
     *
     * @param request A boost::shared_ptr to a dcWebservice::Request object.
     * @returns void
     */
    void _populateParameters(RequestPtr request);

    /*
     * Looks for HTTP headers in the environment parameters present in the
     * FastCGI request, and loads them in the the dcWebservice::Request httpHeaders
     * map.
     *
     * @param fcgiRequest A populated FastCGI object.
     * @param request A boost::shared_ptr to a dcWebservice::Request object.
     * @returns void
     *
     */
    void _populateHttpHeaders(FCGX_Request& fcgiRequest, RequestPtr request);
};

}

#endif // REQUEST_BUILDER_H
