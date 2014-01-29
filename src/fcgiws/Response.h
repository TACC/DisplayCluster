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

#ifndef RESPONSE_H
#define RESPONSE_H

#include <map>
#include <string>
#include <iostream>

#include <boost/shared_ptr.hpp>

namespace fcgiws
{

struct Response;
typedef boost::shared_ptr<Response> ResponsePtr;

/**
 * Structure representing a HTTP reponse message as specified in
 * http://tools.ietf.org/search/rfc2616
 */
struct Response
{
    /**
     * HTTP status code as defined in RFC 2616.
     */
    unsigned int statusCode;

    /**
     * HTTP status message as defined in RFC 2616.
     */
    std::string statusMsg;

    /**
     * HTTP response body, as defined in RFC 2616.
     */
    std::string body;

    /**
     * HTTP response headers as defined in RFC 2616.
     */
    std::map<std::string, std::string> httpHeaders;

    /**
     * Serialize the object into a String contaning a RFC 2616 compliant
     * HTTP response message.
     *
     * @returns A new string representing a HTTP response message.
     */
    std::string serialize() const;

    /*
     * Singletons for 200, 404, and 500 HTT responses
     * See http://tools.ietf.org/search/rfc2616 for more details
     */
    static const Response OK;
    static const Response NotFound;
    static const Response ServerError;
};

std::ostream& operator<<(std::ostream& os, const Response& obj);

}

#endif // RESPONSE_H
