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

#include <sstream>

#include "Response.h"

namespace {
const std::string HTTP_VERSION = "HTTP/1.1";
const std::string SP = " ";
const std::string CRLF = "\r\n";
}

namespace dcWebservice
{

Response::Response(unsigned int code, std::string  msg, std::string body)
    : statusCode(code),
      statusMsg(msg),
      body(body)
{}

ConstResponsePtr Response::OK()
{
    static ConstResponsePtr response(new Response(200, "OK",
		       "{\"code\":\"200\", \"msg\":\"OK\"}"));
    return response;
}


ConstResponsePtr Response::NotFound()
{ 
    static ConstResponsePtr response(new Response(404, "Not Found",
		       "{\"code\":\"404\", \"msg\":\"Not Found\"}"));
    return response;
}

ConstResponsePtr Response::ServerError()
{ 
    static ConstResponsePtr response(new Response(500, "Internal Server Error",
		       "{\"code\":\"500\", \"msg\":\"Internal Server Error\"}"));
    return response;
}

std::string Response::serialize() const
{
    std::stringstream ss;
    ss << HTTP_VERSION << SP << statusCode << SP << statusMsg << CRLF;

    for(std::map<std::string, std::string>::const_iterator it = httpHeaders.begin() ;
        it != httpHeaders.end() ; ++it)
    {
        ss << it->first << ": " << it->second << CRLF;
    }

    ss << "Content-Length: " << body.length() << CRLF;
    ss << "Status: " << statusCode << SP << statusMsg << CRLF;
    ss << CRLF;
    ss << body;
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const Response& obj)
{
    os << obj.serialize();
    return os;
}

bool operator==(const Response& lhs, const Response& rhs)
{
    return (lhs.statusCode == rhs.statusCode &&
	    lhs.statusMsg == rhs.statusMsg &&
	    lhs.body == rhs.body);
}

}
