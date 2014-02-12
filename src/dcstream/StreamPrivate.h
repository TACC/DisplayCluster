/*********************************************************************/
/* Copyright (c) 2013-2014, EPFL/Blue Brain Project                  */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
/*                     Stefan.Eilemann@epfl.ch                       */
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

#ifndef DCSTREAMPRIVATE_H
#define DCSTREAMPRIVATE_H

#include <string>

#include "Event.h"
#include "MessageHeader.h"
#include "ImageSegmenter.h"
#include "Socket.h" // member

class QString;

namespace dc
{

struct PixelStreamSegment;
struct PixelStreamSegmentParameters;

/**
 * Private implementation for the Stream class.
 */
class StreamPrivate
{
public:
    /**
     * Create a new stream and open a new connection to the DisplayCluster.
     *
     * It can be a hostname like "localhost" or an IP in string format,
     * e.g. "192.168.1.83" This method must be called by all Streams sharing a
     * common identifier before any of them starts sending images.
     *
     * @param name the unique stream name
     * @param address Address of the target DisplayCluster instance.
     * @return true if the connection could be established
     */
    StreamPrivate( const std::string& name, const std::string& address );

    ~StreamPrivate();

    /** The stream identifier. */
    const std::string name_;

    /** The communication socket instance */
    Socket dcSocket_;

    /** The image segmenter */
    ImageSegmenter imageSegmenter_;

    /** Has a successful event registration reply been received */
    bool registeredForEvents_;

    /**
     * Close the stream.
     * @return true if the connection could be terminated or the Stream was not connected, false otherwise
     */
    bool close();

    /**
     * Send an existing PixelStreamSegment via the DcSocket.
     * @param socket The DcSocket instance
     * @param segment A pixel stream segement with valid parameters and imageData
     * @param senderName Used to identifiy the sender on the receiver side
     * @return true if the message could be sent
     */
    bool sendPixelStreamSegment(const PixelStreamSegment& segment);

    /**
     * Send a command to the wall
     * @param command A command string formatted by the Command class.
     * @return true if the request could be sent, false otherwise.
     */
    bool sendCommand(const QString& command);
};

}
#endif // DCSTREAMPRIVATE_H
