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

#include "Server.h"

#include "Response.h"
#include "Request.h"

#include <sys/socket.h>
#include <netdb.h>
#include <cstdio>

namespace dcWebservice
{

Server::Server() : _requestBuilder(new RequestBuilder()),
    _fcgi(new FastCGIWrapper())
{}

bool Server::addHandler(const std::string& pattern, HandlerPtr handler)
{
    return _mapper.addHandler(pattern, handler);
}

bool Server::run(const unsigned int port)
{
    if(!_fcgi->init(port))
        return false;

    while(_fcgi->accept())
    {
        _processRequest();
    }
    return true;
}

void Server::_sendResponse(const Response& response)
{
    _fcgi->write(response.serialize());
}

void Server::_processRequest()
{
    RequestPtr request = _requestBuilder->buildRequest(*_fcgi->getRequest());
    if(!request)
    {
        _sendResponse(*Response::ServerError());
        return;
    }

    const Handler& handler = _mapper.getHandler(request->resource);

    ConstResponsePtr response = handler.handle(*request);
    if(!response)
    {
        _sendResponse(*Response::ServerError());
        return;
    }

    _sendResponse(*response);
}

bool Server::stop()
{
    return _fcgi->stop();
}

}
