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

#ifndef PIXELSTREAMDISPATCHER_H
#define PIXELSTREAMDISPATCHER_H

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <map>

#include "PixelStreamSegment.h"
#include "PixelStreamBuffer.h"

//#define USE_TIMER

using dc::PixelStreamSegment;

typedef std::map<QString, PixelStreamBuffer> StreamBuffers;

/**
 * Gather PixelStream Segments from multiple sources and dispatch them to Wall processes through MPI
 */
class PixelStreamDispatcher : public QObject
{
    Q_OBJECT

public:
    /** Construct a dispatcher */
    PixelStreamDispatcher();

public slots:
    /**
     * Add a source of Segments for a Stream
     *
     * @param uri Identifier for the Stream
     * @param sourceIndex Identifier for the source in this stream
     */
    void addSource(const QString uri, const size_t sourceIndex);

    /**
     * Add a source of Segments for a Stream
     *
     * @param uri Identifier for the Stream
     * @param sourceIndex Identifier for the source in this stream
     */
    void removeSource(const QString uri, const size_t sourceIndex);

    /**
     * Process a new Segement
     *
     * @param uri Identifier for the Stream
     * @param sourceIndex Identifier for the source in this stream
     */
    void processSegment(const QString uri, const size_t sourceIndex, PixelStreamSegment segment);

    /**
     * The given source has finished sending segments for the current frame
     *
     * @param uri Identifier for the Stream
     * @param sourceIndex Identifier for the source in this stream
     */
    void processFrameFinished(const QString uri, const size_t sourceIndex);

    /**
     * Delete an entire stream
     *
     * @param uri Identifier for the Stream
     */
    void deleteStream(const QString uri);

signals:
    /**
     * Notify that a PixelStream has been opened
     *
     * @param uri Identifier for the Stream
     * @param width Width of the stream
     * @param height Height of the stream
     */
    void openPixelStream(QString uri, int width, int height);

    /**
     * Notify that a pixel stream has been deleted
     *
     * @param uri Identifier for the Stream
     */
    void deletePixelStream(QString uri);

#ifndef USE_TIMER
    /** @internal */
    void dispatchFramesSignal();
#endif

private slots:
    void dispatchFrames();

private:
    // The buffers for each URI
    StreamBuffers streamBuffers_;

    void sendPixelStreamSegments(const std::vector<PixelStreamSegment> &segments, const QString& uri);

#ifdef USE_TIMER
    QTimer sendTimer_;
#else
    QDateTime lastFrameSent_;
#endif
};

#endif // PIXELSTREAMDISPATCHER_H
