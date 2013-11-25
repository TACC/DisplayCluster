/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
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

#ifndef DCSTREAM_H
#define DCSTREAM_H

#include <string>

#include "Event.h"
#include "ImageWrapper.h"
#include "NonCopyable.h"

namespace dc
{

class StreamPrivate;

/**
 * Stream visual data to a DisplayCluster application.
 *
 * A Stream can be subdivided into one or more images.
 * This allows to have different applications each responsible for sending one part of the global image.
 *
 * The methods in this class are reentrant (all instances are independant) but are not thread-safe.
 */
class Stream : public NonCopyable
{
public:
    /**
     * Open a new connection to the DisplayCluster application.
     *
     * The user can check if the connection was successfully established with isConnected().
     * @param name An identifier for the stream which cannot be null.
     *        The DisplayCluster application creates a window for the Stream using this name.
     *        Different Streams can contribute to a single window by using the same name.
     *        All the Streams which use the same name must be created before any of them starts sending images.
     * @param address Address of the target DisplayCluster instance.
     *        It can be a hostname like "localhost" or an IP in string format, e.g. "192.168.1.83".
     * @version 1.0
     */
    Stream(const std::string& name, const std::string& address);

    /** Destruct the Stream, closing the connection. @version 1.0 */
    virtual ~Stream();

    /** @return true if the stream is connected, false otherwise. @version 1.0 */
    bool isConnected() const;

    /**
     * Send an image.
     *
     * @param image The image to send
     * @return true if the image data could be sent, false otherwise
     * @version 1.0
     */
    bool send(const ImageWrapper& image);

    /**
     * Notify that all the images for this frame have been sent.
     *
     * This method must be called everytime this Stream instance has finished sending
     * its image(s) for the current frame.
     * The receiver will display the images once all senders using the same stream identifier
     * have finished a frame.
     * @see send()
     * @version 1.0
     */
    bool finishFrame();

    /**
     * Register to receive Events for this Stream.
     *
     * After registering for events, the DisplayCluster master application will send updates
     * whenever a user is interacting with this Stream's window.
     * The user of the library can retrieve them using hasEvent() and getEvent().
     * The current registration status can be checked with isRegisterdForEvents().
     * @param exclusive Binds only one stream source for the same name
     * @return true if the registration could be established, false otherwise.
     * @note This call will fail until the window has been created, which should happen some time after the
     * first finishFrame() has succeeded.
     * @version 1.0
     */
    bool registerForEvents(const bool exclusive = false);

    /**
     * Is this stream registered to receive events.
     *
     * Check if the stream has already been register to receive events with registerForEvents().
     * @return true after the DisplayCluster application has acknowledged the registration request, false otherwise
     * @version 1.0
     */
    bool isRegisterdForEvents() const;

    /**
     * Get the native descriptor for the data stream.
     *
     * This descriptor can for instance be used by poll() on UNIX systems.
     * Having this descriptor lets a Stream class user detect when the Stream has received any data.
     * The user can the use query the state of the Stream, for example using hasEvent(),
     * and process the events accordingly.
     * @return The native descriptor if available; otherwise returns -1.
     * @version 1.0
     */
    int getDescriptor() const;

    /**
     * Check if a new Event is available.
     *
     * This method is non-blocking.
     * @return True if an Event is available, false otherwise
     * @note Use this method prior to calling getEvent(), for example as the condition
     * for a while() loop to process all pending events.
     * @version 1.0
     */
    bool hasEvent() const;

    /**
     * Get the next Event.
     *
     * This method is blocking.
     * @return The next Event in the queue if available, otherwise an empty (default) Event.
     * @note users are advised to check if an Event is available with
     *       hasEvent() before calling this method.
     * @version 1.0
     */
    Event getEvent();

private:
    StreamPrivate* impl_;
};

}

#endif // DCSTREAM_H
