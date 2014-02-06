/*********************************************************************/
/* Copyright (c) 2013-2014, EPFL/Blue Brain Project                  */
/*                          Raphael Dumusc <raphael.dumusc@epfl.ch>  */
/*                          Stefan.Eilemann@epfl.ch                  */
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

class Application;

namespace dc
{

class StreamPrivate;

/**
 * Stream visual data to a DisplayCluster application.
 *
 * A Stream can be subdivided into one or more images.  This allows to have
 * different applications each responsible for sending one part of the global
 * image.
 *
 * The methods in this class are reentrant (all instances are independant) but are not thread-safe.
 */
class Stream
{
public:
    /**
     * Open a new connection to the DisplayCluster application.
     *
     * The user can check if the connection was successfully established with
     * isConnected(). The DisplayCluster application creates a window for the
     * Stream using the given name as an identifier.
     *
     * Different Streams can contribute to a single window by using the same
     * name. All the Streams which contribute to the same window should be
     * created before any of them starts sending images.
     *
     * @param name An identifier for the stream which cannot be empty.
     * @param address Address of the target DisplayCluster instance, can be a
     *                hostname like "localhost" or an IP in string format like
     *                "192.168.1.83".
     * @version 1.0
     */
    Stream(const std::string& name, const std::string& address);

    /** Destruct the Stream, closing the connection. @version 1.0 */
    virtual ~Stream();

    /** @return true if the stream is connected, false otherwise. @version 1.0*/
    bool isConnected() const;

    /**
     * Send an image.
     *
     * @param image The image to send
     * @return true if the image data could be sent, false otherwise
     * @version 1.0
     * @sa finishFrame()
     */
    bool send(const ImageWrapper& image);

    /**
     * Notify that all the images for this frame have been sent.
     *
     * This method must be called everytime this Stream instance has finished
     * sending its image(s) for the current frame. The receiver will display
     * the images once all the senders which use the same name have finished a
     * frame.
     *
     * @see send()
     * @version 1.0
     */
    bool finishFrame();

    /**
     * Register to receive Events.
     *
     * After registering, the DisplayCluster master application will send Events
     * whenever a user is interacting with this Stream's window.
     *
     * Registation is only possible after a window for the stream has been
     * created on the DisplayWall. A window is first created when all Streams
     * that use the same identifier have sent the first frame and called
     * finishFrame().
     *
     * Events can be retrieved using hasEvent() and getEvent().
     *
     * The current registration status can be checked with
     * isRegisteredForEvents().
     *
     * This method is synchronous and waits for a registration reply from the
     * DisplayWall before returning.
     *
     * @param exclusive Binds only one stream source for the same name
     * @return true if the registration could be or was already established.
     * @version 1.0
     */
    bool registerForEvents(const bool exclusive = false);

    /**
     * Is this stream registered to receive events.
     *
     * Check if the stream has already successfully registered with
     * registerForEvents().
     *
     * @return true after the DisplayCluster application has acknowledged the
     *         registration request, false otherwise
     * @version 1.0
     */
    bool isRegisteredForEvents() const;

    /**
     * Get the native descriptor for the data stream.
     *
     * This descriptor can for instance be used by poll() on UNIX systems.
     * Having this descriptor lets a Stream class user detect when the Stream
     * has received any data. The user can the use query the state of the
     * Stream, for example using hasEvent(), and process the events accordingly.
     *
     * @return The native descriptor if available; otherwise returns -1.
     * @version 1.0
     */
    int getDescriptor() const;

    /**
     * Check if a new Event is available.
     *
     * This method is non-blocking. Use this method prior to calling getEvent(),
     * for example as the condition for a while() loop to process all pending
     * events.
     *
     * @return True if an Event is available, false otherwise
     * @version 1.0
     */
    bool hasEvent() const;

    /**
     * Get the next Event.
     *
     * This method is sychronous and waits until an Event is available before
     * returning (or a 1 second timeout occurs).
     *
     * Check if an Event is available with hasEvent() before calling this
     * method.
     *
     * @return The next Event if available, otherwise an empty (default) Event.
     * @version 1.0
     */
    Event getEvent();

private:
    /** Disable copy constructor. */
    Stream( const Stream& );

    /** Disable assignment operator. */
    const Stream& operator = ( const Stream& );

    StreamPrivate* impl_;

    friend class ::Application;
};

}

#endif // DCSTREAM_H
