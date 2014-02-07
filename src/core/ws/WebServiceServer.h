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

#ifndef WEBSERVICESERVER_H
#define WEBSERVICESERVER_H

#include <QThread>

#include "dcWebservice/types.h"

/**
 * A Qt wrapper to run the dcWebservice::Server in a QThread.
 */
class WebServiceServer : public QThread
{
    Q_OBJECT
public:
    /** Constructor */
    WebServiceServer(const unsigned int port, QObject *parent = 0);

    /** Destructor */
    ~WebServiceServer();

    /**
     * Registers a request handler with a particular regular expression.
     *
     * When the URL of an incoming request matches the regular expression
     * the handler is invoked.
     *
     * @param pattern A regular expression.
     * @param handler A request handler. If the handler is a QObject, it should be moved
     *        to this thread before making any signal/slot connections.
     * @return true if the handler was registered succesfully, false otherwise,
     *         for instance if the regular expression is not valid.
     */
    bool addHandler(const std::string& pattern, dcWebservice::HandlerPtr handler);

    /**
     * Stop the server. This method is thread-safe.
     */
    bool stop();

protected:
    /** @overload Start the server. */
    virtual void run();

private:
    dcWebservice::Server* server_;
    unsigned int port_;
};

#endif // WEBSERVICESERVER_H
