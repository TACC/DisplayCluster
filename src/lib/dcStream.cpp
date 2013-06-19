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

#include "dcStream.h"
#include "../NetworkProtocol.h"
#include "../MessageHeader.h"
#include "../ParallelPixelStreamSegmentParameters.h"
#include "../log.h"
#include <QtNetwork/QTcpSocket>
#include <QtCore>
#include <cmath>
#include <turbojpeg.h>
#include <algorithm>
#include <unistd.h>

// default to undefined frame index
int g_dcStreamFrameIndex = FRAME_INDEX_UNDEFINED;

// all current source indices for each stream name
typedef std::map< std::string, std::vector<int> > SourcesIndices;
typedef std::map< DcSocket*, SourcesIndices > StreamSources;
StreamSources g_dcStreamSourceIndices;

struct DcImage {
    unsigned char * imageBuffer;
    int width;
    int pitch;
    int height;
    PIXEL_FORMAT pixelFormat;
    char * jpegData;
    int jpegSize;
};

// enum PIXEL_FORMAT { RGB, RGBA, ARGB, BGR, BGRA, ABGR };
int dcBytesPerPixel[] = { 3, 4, 4, 3, 4, 4 };

DcImage dcStreamComputeJpegMapped(const DcImage & dcImage);


DcSocket * dcStreamConnect(const char * hostname)
{
    DcSocket * socket = new QTcpSocket();

    // open connection
    socket->connectToHost(hostname, 1701);

    if(socket->waitForConnected() != true)
    {
        put_flog(LOG_ERROR, "could not connect to host %s", hostname);

        return NULL;
    }

    // handshake
    while(socket->waitForReadyRead() && socket->bytesAvailable() < (int)sizeof(int32_t))
    {
    #ifndef _WIN32
        usleep(10);
    #endif
    }

    int32_t protocolVersion = -1;
    socket->read((char *)&protocolVersion, sizeof(int32_t));

    if(protocolVersion != NETWORK_PROTOCOL_VERSION)
    {
        socket->disconnectFromHost();

        put_flog(LOG_ERROR, "unsupported protocol version %i != %i", protocolVersion, NETWORK_PROTOCOL_VERSION);

        return NULL;
    }

    return socket;
}

void dcStreamDisconnect(DcSocket * socket)
{
    if( !socket || socket->state() != QAbstractSocket::ConnectedState )
        return;

    StreamSources::const_iterator i = g_dcStreamSourceIndices.find( socket );
    if( i != g_dcStreamSourceIndices.end( ))
    {
        // header
        MessageHeader mh;
        mh.size = 0;
        mh.type = MESSAGE_TYPE_QUIT;

        for( SourcesIndices::const_iterator j = i->second.begin();
             j != i->second.end(); ++j )
        {
            // add the truncated URI to the header
            const size_t len = j->first.copy(mh.uri, MESSAGE_HEADER_URI_LENGTH - 1);
            mh.uri[len] = '\0';

            // send the header
            int sent = socket->write((const char *)&mh, sizeof(MessageHeader));
            while(sent < (int)sizeof(MessageHeader))
            {
                sent += socket->write((const char *)&mh + sent, sizeof(MessageHeader) - sent);
            }

            // wait for acknowledgment
            while(socket->waitForReadyRead() && socket->bytesAvailable() < 3)
            {
        #ifndef _WIN32
                usleep(10);
        #endif
            }

            socket->read(3);
        }
    }

    delete socket;
    socket = NULL;
}

void dcStreamReset(DcSocket * socket)
{
    StreamSources::iterator i = g_dcStreamSourceIndices.find( socket );
    if( i == g_dcStreamSourceIndices.end( ))
        return;

    for( SourcesIndices::iterator j = i->second.begin(); j != i->second.end();
         ++j )
    {
        const std::string& name = j->first;
        for( size_t k = 0; k < j->second.size(); ++k )
        {
            int sourceIndex = j->second[k];

            // blank parameters object
            DcStreamParameters parameters = dcStreamGenerateParameters(name, sourceIndex, 0, 0, 0, 0, 0, 0);

            // send the blank parameters object
            // this will trigger remote deletion of this segment
            dcStreamSendJpeg(socket, parameters, NULL, 0);
        }
    }

    // clear the current source indices for each stream name
    g_dcStreamSourceIndices.erase( i );
}

DcStreamParameters dcStreamGenerateParameters(std::string name, int sourceIndex, int x, int y, int width, int height, int totalWidth, int totalHeight)
{
    DcStreamParameters parameters;

    parameters.name = name;
    parameters.sourceIndex = sourceIndex;
    parameters.x = x;
    parameters.y = y;
    parameters.width = width;
    parameters.height = height;
    parameters.totalWidth = totalWidth;
    parameters.totalHeight = totalHeight;

    return parameters;
}

std::vector<DcStreamParameters> dcStreamGenerateParameters(std::string name, int firstSourceIndex, int nominalSegmentWidth, int nominalSegmentHeight, int x, int y, int width, int height, int totalWidth, int totalHeight)
{
    // segment dimensions will be approximately nominalSegmentWidth x nominalSegmentHeight

    // number of subdivisions in each dimension
    int numSubdivisionsX = (int)floor((float)width / (float)nominalSegmentWidth + 0.5);
    int numSubdivisionsY = (int)floor((float)height / (float)nominalSegmentHeight + 0.5);

    // now, create parameters for each segment
    std::vector<DcStreamParameters> parameters;

    for(int i=0; i<numSubdivisionsX; i++)
    {
        for(int j=0; j<numSubdivisionsY; j++)
        {
            DcStreamParameters p;

            p.name = name;
            p.sourceIndex = firstSourceIndex;
            p.x = x + i * (int)((float)width / (float)numSubdivisionsX);
            p.y = y + j * (int)((float)height / (float)numSubdivisionsY);
            p.width = (int)((float)width / (float)numSubdivisionsX);
            p.height = (int)((float)height / (float)numSubdivisionsY);
            p.totalWidth = totalWidth;
            p.totalHeight = totalHeight;

            parameters.push_back(p);

            firstSourceIndex++;
        }
    }

    return parameters;
}

bool dcStreamSend(DcSocket * socket, unsigned char * imageBuffer, int imageX, int imageY, int imageWidth, int imagePitch, int imageHeight, PIXEL_FORMAT pixelFormat, DcStreamParameters parameters)
{
    // compute imagePitch if necessary, assuming imageBuffer isn't padded
    if(imagePitch == 0)
    {
        imagePitch = imageWidth * dcBytesPerPixel[pixelFormat];
    }

    // compute JPEG from imageBuffer corresponding to parameters
    unsigned char * segmentImageBuffer = imageBuffer + (parameters.y - imageY)*imagePitch + (parameters.x - imageX)*dcBytesPerPixel[pixelFormat];

    char * jpegData = NULL;
    int jpegSize = 0;

    bool success = dcStreamComputeJpeg(segmentImageBuffer, parameters.width, imagePitch, parameters.height, pixelFormat, &jpegData, jpegSize);

    if(success == false)
    {
        free(jpegData);
        return false;
    }

    success = dcStreamSendJpeg(socket, parameters, jpegData, jpegSize);

    free(jpegData);
    return success;
}

bool dcStreamSend(DcSocket * socket, unsigned char * imageBuffer, int imageX, int imageY, int imageWidth, int imagePitch, int imageHeight, PIXEL_FORMAT pixelFormat, std::vector<DcStreamParameters> parameters)
{
    // compute imagePitch if necessary, assuming imageBuffer isn't padded
    if(imagePitch == 0)
    {
        imagePitch = imageWidth * dcBytesPerPixel[pixelFormat];
    }

    // compute JPEGs from imageBuffer corresponding to parameters vector
    std::vector<DcImage> dcImages;

    for(unsigned int i=0; i<parameters.size(); i++)
    {
        DcImage d;

        // imageBuffer coordinates have the origin at the bottom-left corner.
        // DisplayCluster's coordinates have the origin at the top-left.
        // a transformation is needed to find the appropriate memory location within the full imageBuffer...
        d.imageBuffer = imageBuffer + imageHeight*imagePitch - (parameters[i].y - imageY + parameters[i].height)*imagePitch + (parameters[i].x - imageX)*dcBytesPerPixel[pixelFormat];

        d.width = parameters[i].width;
        d.pitch = imagePitch;
        d.height = parameters[i].height;
        d.pixelFormat = pixelFormat;
        d.jpegData = NULL;
        d.jpegSize = 0;

        dcImages.push_back(d);
    }

    // create JPEGs for each segment, in parallel

    dcImages = QtConcurrent::blockingMapped<std::vector<DcImage> >(dcImages, &dcStreamComputeJpegMapped);

    // send each segment, and return true if we were successful for all segments
    bool allSuccess = true;

    for(unsigned int i=0; i<dcImages.size(); i++)
    {
        // jpegSize == 0 indicates an error
        if(dcImages[i].jpegSize == 0)
        {
            allSuccess = false;
        }
        else
        {
            bool sendSuccess = dcStreamSendJpeg(socket, parameters[i], dcImages[i].jpegData, dcImages[i].jpegSize);

            if(sendSuccess == false)
            {
                allSuccess = false;
            }
        }

        free(dcImages[i].jpegData);
    }

    return allSuccess;
}

bool dcStreamSendJpeg(DcSocket * socket, DcStreamParameters parameters, const char * jpegData, int jpegSize)
{
    if(socket == NULL)
    {
        put_flog(LOG_ERROR, "socket is NULL");

        return false;
    }

    if(socket->state() != QAbstractSocket::ConnectedState)
    {
        put_flog(LOG_ERROR, "socket is not connected");

        return false;
    }

    // send the parameters and image data
    MessageHeader mh;
    mh.size = sizeof(ParallelPixelStreamSegmentParameters) + jpegSize;
    mh.type = MESSAGE_TYPE_PARALLEL_PIXELSTREAM;

    // add the truncated URI to the header
    size_t len = parameters.name.copy(mh.uri, MESSAGE_HEADER_URI_LENGTH - 1);
    mh.uri[len] = '\0';

    // send the header
    int sent = socket->write((const char *)&mh, sizeof(MessageHeader));

    while(sent < (int)sizeof(MessageHeader))
    {
        sent += socket->write((const char *)&mh + sent, sizeof(MessageHeader) - sent);
    }

    // send the message

    // part 1: parameters
    ParallelPixelStreamSegmentParameters p;

    p.sourceIndex = parameters.sourceIndex;
    p.frameIndex = g_dcStreamFrameIndex;
    p.x = parameters.x;
    p.y = parameters.y;
    p.width = parameters.width;
    p.height = parameters.height;
    p.totalWidth = parameters.totalWidth;
    p.totalHeight = parameters.totalHeight;

    sent = socket->write((const char *)&(p), sizeof(ParallelPixelStreamSegmentParameters));

    while(sent < (int)sizeof(ParallelPixelStreamSegmentParameters))
    {
        sent += socket->write((const char *)&(p) + sent, sizeof(ParallelPixelStreamSegmentParameters) - sent);
    }

    // part 2: image data
    if(jpegSize > 0)
    {
        sent = socket->write(jpegData, jpegSize);

        while(sent < jpegSize)
        {
            sent += socket->write(jpegData + sent, jpegSize - sent);
        }
    }

    // wait for acknowledgment
    while(socket->waitForReadyRead() && socket->bytesAvailable() < 3)
    {
#ifndef _WIN32
        usleep(10);
#endif
    }

    socket->read(3);

    SourcesIndices& indices = g_dcStreamSourceIndices[socket];
    // make sure this sourceIndex is in the vector of current source indices for this stream name
    if(count(indices[parameters.name].begin(), indices[parameters.name].end(), parameters.sourceIndex) == 0)
    {
        indices[parameters.name].push_back(parameters.sourceIndex);
    }

    return true;
}

bool dcStreamComputeJpeg(unsigned char * imageBuffer, int width, int pitch, int height, PIXEL_FORMAT pixelFormat, char ** jpegData, int & jpegSize)
{
    // use libjpeg-turbo for JPEG conversion

    // use a new handle each time for thread-safety
    tjhandle tjHandle = tjInitCompress();

    // compute pitch if necessary, assuming imageBuffer isn't padded
    if(pitch == 0)
    {
        pitch = width * dcBytesPerPixel[pixelFormat];
    }

    // map pixel format to the libjpeg-turbo equivalent
    int tjPixelFormat;

    // enum PIXEL_FORMAT { RGB, RGBA, ARGB, BGR, BGRA, ABGR };
    switch(pixelFormat)
    {
        case RGB:
            tjPixelFormat = TJPF_RGB;
            break;
        case RGBA:
            tjPixelFormat = TJPF_RGBX;
            break;
        case ARGB:
            tjPixelFormat = TJPF_XRGB;
            break;
        case BGR:
            tjPixelFormat = TJPF_BGR;
            break;
        case BGRA:
            tjPixelFormat = TJPF_BGRX;
            break;
        case ABGR:
            tjPixelFormat = TJPF_XBGR;
            break;
        default:
            put_flog(LOG_ERROR, "unknown pixel format");
            return false;
    }

    unsigned char ** tjJpegBuf;
    unsigned char * tjJpegBufPtr = NULL;
    tjJpegBuf = &tjJpegBufPtr;
    unsigned long tjJpegSize = 0;
    int tjJpegSubsamp = TJSAMP_444;
    int tjJpegQual = 75;
    int tjFlags = TJFLAG_BOTTOMUP;

    int success = tjCompress2(tjHandle, imageBuffer, width, pitch, height, tjPixelFormat, tjJpegBuf, &tjJpegSize, tjJpegSubsamp, tjJpegQual, tjFlags);

    if(success != 0)
    {
        put_flog(LOG_ERROR, "libjpeg-turbo image conversion failure");

        jpegSize = 0;

        // destroy libjpeg-turbo handle
        tjDestroy(tjHandle);

        return false;
    }

    // move the JPEG buffer to our own memory and free the libjpeg-turbo allocated memory
    *jpegData = (char *)realloc((void *)*jpegData, tjJpegSize);
    memcpy(*jpegData, tjJpegBufPtr, tjJpegSize);
    tjFree(tjJpegBufPtr);

    jpegSize = tjJpegSize;

    // destroy libjpeg-turbo handle
    tjDestroy(tjHandle);

    return true;
}

void dcStreamIncrementFrameIndex()
{
    g_dcStreamFrameIndex++;
}

bool dcStreamSendSVG(DcSocket * socket, std::string name, const char * svgData, int svgSize)
{
    if(socket == NULL)
    {
        put_flog(LOG_ERROR, "socket is NULL");

        return false;
    }

    if(socket->state() != QAbstractSocket::ConnectedState)
    {
        put_flog(LOG_ERROR, "socket is not connected");

        return false;
    }

    // send the parameters and image data
    MessageHeader mh;
    mh.size = svgSize;
    mh.type = MESSAGE_TYPE_SVG_STREAM;

    // add the truncated URI to the header
    size_t len = name.copy(mh.uri, MESSAGE_HEADER_URI_LENGTH - 1);
    mh.uri[len] = '\0';

    // send the header
    int sent = socket->write((const char *)&mh, sizeof(MessageHeader));

    while(sent < (int)sizeof(MessageHeader))
    {
        sent += socket->write((const char *)&mh + sent, sizeof(MessageHeader) - sent);
    }

    // send the message

    // part 1: image data
    if(svgSize > 0)
    {
        sent = socket->write(svgData, svgSize);

        while(sent < svgSize)
        {
            sent += socket->write(svgData + sent, svgSize - sent);
        }
    }

    // wait for acknowledgment
    while(socket->waitForReadyRead() && socket->bytesAvailable() < 3)
    {
#ifndef _WIN32
        usleep(10);
#endif
    }

    socket->read(3);

    return true;
}

DcImage dcStreamComputeJpegMapped(const DcImage & dcImage)
{
    DcImage newDcImage = dcImage;

    dcStreamComputeJpeg(newDcImage.imageBuffer, newDcImage.width, newDcImage.pitch, newDcImage.height, newDcImage.pixelFormat, &newDcImage.jpegData, newDcImage.jpegSize);

    return newDcImage;
}
