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

#ifndef FASTCGI_WRAPPER_H
#define FASTCGI_WRAPPER_H

#include <string>
#include <boost/scoped_ptr.hpp>

#include <fcgiapp.h>

namespace dcWebservice
{

/**
 * Wrapper for the FastCGI library. All methods are virtual to facilitate tests.
 */
class FastCGIWrapper
{
public:
    /**
     * Constructor
     */
    FastCGIWrapper();

    /**
     * Destructor
     */
    virtual ~FastCGIWrapper();

    /**
     * Initialize the FCGI library. After this method is called, it is assumed
     * that it is safe to call accept() to receive requests.
     *
     * @param socket An id identifying the socket where requests are going to be
     * received.
     * @param nbOfConnections number of connections to enqueue in the socket.
     * @returns true if initialization succeeds, false otherwise.
     */
    virtual bool init(const unsigned int port, const unsigned int nbOfConnections = 5);

    /**
     * Blocks until a request arrives.
     *
     * @returns true if a request is received successfully, false otherwise.
     */
    virtual bool accept();

    /**
     * Get the actual FCGX_Request object associated with this wrapper.
     *
     * @returns The FCGX_Request object associated with this wrapper.
     */
    virtual FCGX_Request* getRequest();

    /**
     * Sends the message back to the requester and finalizes the request.
     *
     * @param msg A string representing a message that is going to be sent
     * to the initiator of the request. Normally this message should be a
     * valid HTTP response message.
     * @returns true upon success, false otherwise.
     */
    virtual bool write(const std::string& msg);

    /**
     * Calling this method will force the interruption of the accept blocking
     * call. This method is meant to be used only during application cleanup.
     *
     * @return true if success, false otherwise.
     */
    virtual bool stop();

private:
    boost::scoped_ptr<FCGX_Request> _request;
    volatile bool _run;
    int _socket;
};

}

#endif // FASTCGI_WRAPPER_H
