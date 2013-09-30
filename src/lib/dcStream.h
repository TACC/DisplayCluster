/*********************************************************************/
/* Copyright (c) 2011 - 2012, The University of Texas at Austin.     */
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

#ifndef DC_STREAM_H
#define DC_STREAM_H

#include <InteractionState.h>
#include <string>
#include <vector>

class DcSocket;

struct DcStreamParameters  {
    std::string name;
    int sourceIndex;
    int x;
    int y;
    int width;
    int height;
    int totalWidth;
    int totalHeight;
    int segmentCount;
    bool compress;  // use jpeg compression or raw image for send, default true
    int quality;    // 0 worst quality, 100 best & lossless jpeg, default 75
};

enum PIXEL_FORMAT { RGB=0, RGBA=1, ARGB=2, BGR=3, BGRA=4, ABGR=5 };

// make a new connection to the DisplayCluster instance on hostname, and
// returns a DcSocket. the user is responsible for closing the socket using
// dcStreamDisconnect().
extern DcSocket * dcStreamConnect(const char * hostname, bool async=true);

// closes a previously opened connection, deleting the socket.
extern void dcStreamDisconnect(DcSocket * socket);

// reset all stream segments associated with this connection.
extern void dcStreamReset(DcSocket * socket);

// generates a new parameter object with the given origin (x, y) and dimensions
// (width, height). the origin (x, y) is relative to the full window represented
// by all streams corresponding to <name>. totalWidth and totalHeight give the
// full dimensions of the window.
extern DcStreamParameters dcStreamGenerateParameters(std::string name, int x, int y, int width, int height, int totalWidth, int totalHeight, bool compress=true);

// generates a vector of parameter objects corresponding to segments contained
// in (x, y, width, height). the parameters' dimensions will be approximately
// nominalSegmentWidth x nominalSegmentHeight. source indices begin with
// firstSourceIndex.
extern std::vector<DcStreamParameters> dcStreamGenerateParameters(std::string name, int firstSourceIndex, int nominalSegmentWidth, int nominalSegmentHeight, int x, int y, int width, int height, int totalWidth, int totalHeight, bool compress=true);

// generates a segment corresponding to parameters from imageBuffer and sends
// it to a DisplayCluster instance over socket. (imageX, imageY, imageWidth,
// imageHeight) give the origin and dimensions of the image relative to the full
// represented by all streams corresponding to <name>. imagePitch is the bytes
// per line in imageBuffer. pixelFormat gives the format of the buffer.
extern bool dcStreamSend(DcSocket * socket, unsigned char * imageBuffer, int size, int imageX, int imageY, int imageWidth, int imagePitch, int imageHeight, PIXEL_FORMAT pixelFormat, DcStreamParameters parameters);

// the same as above, excepts sends a group of segments corresponding to the
// given vector of parameters. compression of segment image data is parallel.
extern bool dcStreamSend(DcSocket * socket, unsigned char * imageBuffer, int imageX, int imageY, int imageWidth, int imagePitch, int imageHeight, PIXEL_FORMAT pixelFormat, std::vector<DcStreamParameters> parameters);

// sends a compressed JPEG image or an uncompressed raw image corresponding to
// parameters and sends it to a DisplayCluster instance over socket. If
// waitForAck is true, this function will block until an acknowledgment is received.
extern bool dcStreamSendImage(DcSocket * socket, DcStreamParameters parameters, const unsigned char * buffer, int size, bool waitForAck=true);

// increment the frame index for all segments sent by this process. this is
// used for frame synchronization.
extern void dcStreamIncrementFrameIndex();

// sends an SVG image with a given name to a DisplayCluster instance over a
// socket. different from the pixel streaming capability, this allows for
// streaming vector-based graphics.
extern bool dcStreamSendSVG(DcSocket * socket, std::string name, const char * svgData, int svgSize);

extern bool dcStreamBindInteraction(DcSocket * socket, std::string name);

// exclusive binds only one stream source for the same name,
// check status with dcStreamHasInteraction()
extern bool dcStreamBindInteractionExclusive(DcSocket * socket, std::string name);

extern InteractionState dcStreamGetInteractionState(DcSocket * socket);

extern int dcSocketDescriptor(DcSocket * socket);

// -1 for no reply yet, 0 for not bound (if exclusive mode),
// 1 for successful bound
extern int dcStreamHasInteraction(DcSocket * socket);

extern bool dcHasNewInteractionState(DcSocket * socket);

#endif
