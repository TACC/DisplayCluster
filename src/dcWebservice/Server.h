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

#ifndef SERVER_H
#define SERVER_H

#include "Mapper.h"
#include "RequestBuilder.h"
#include "Handler.h"
#include "FastCGIWrapper.h"

#include <boost/scoped_ptr.hpp>

namespace dcWebservice
{

/**
 * FastCGI application server.
 *
 * The Server class has two main puposes, listening for incomming requests
 * and dispatching them to handlers.
 *
 * Users of this class must register handlers using the addHandler method.
 * Upon reception of a request the Server looks for the Handler mapped to
 * the URL of the request.
 *
 * The port in which the server listens for incoming requests is configurable.
 */
class Server
{
public:
    /**
     * Constructor.
     */
    Server();

    /**
     * Registers a request handler with a particular regular expression.
     *
     * When the URL of an incoming request matches the regular expression
     * the handler is invoked.
     *
     * @param pattern A regular expression.
     * @param handler A request handler.
     * @returns true if the handler was registered succesfully, false otherwise,
     *   for instance if the regular expression is not valid.
     */
    bool addHandler(const std::string& pattern, HandlerPtr handler);

    /**
     * Binds a TCP socket in the port specified and starts listening for
     * incoming requests. Runs entirely in the same thread in which it is
     * called. This method blocks until a call to stop() is executed.
     *
     * @param port The port used in the creation of the TCP socket.
     * @returns true upon successful completion, false otherwise.
     */
    bool run(const unsigned int port);

    /**
     * Stops the Server request processing loop. If the server is running
     * this method causes the Server to stop processing.
     *
     * @returns true upcon successful completion, false otherwise.
     */
    bool stop();

#ifdef TESTS
    void fireProcessing()
    {
        _processRequest();
    }
    void setMapper(Mapper mapper)
    {
        _mapper = mapper;
    }
    void setRequestBuilder(RequestBuilder* builder)
    {
        _requestBuilder.reset(builder);
    }
    void setFastCGIWrapper(FastCGIWrapper* wrapper)
    {
        _fcgi.reset(wrapper);
    }
#endif

private:
    Mapper _mapper;
    boost::scoped_ptr<RequestBuilder> _requestBuilder;
    boost::scoped_ptr<FastCGIWrapper> _fcgi;

    void _sendResponse(const Response& response);
    void _processRequest();
};

}

#endif // SERVER_H
