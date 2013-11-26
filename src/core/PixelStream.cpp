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

#include "PixelStream.h"
#include "globals.h"
#include "ContentWindowManager.h"
#include "DisplayGroupManager.h"
#include "MainWindow.h"
#include "GLWindow.h"
#include "log.h"

#include "PixelStreamSegmentRenderer.h"
#include "PixelStreamSegmentDecoder.h"

#include "PixelStreamSegmentParameters.h"

#define MAX_FRAME_QUEUE_SIZE 5

PixelStream::PixelStream(const QString &uri)
    : uri_(uri)
    , width_(0)
    , height_ (0)
    , decodingStarted_(false)
{
}

void PixelStream::getDimensions(int &width, int &height) const
{
    width = width_;
    height = height_;
}

void PixelStream::preRenderUpdate()
{
    if(!isDecodingInProgress())
    {
        // Update the texture which need an update and which are available
        updateVisibleTextures();

        // Always keep at least one frame in the queue in case the sender stops sending
        // and we need to decode previously hidden segments
        if (frameBuffer_.size() > 1)
        {
            nextFrame();
        }
        // Start decoding the textures which are not decoded but needed
        decodeVisibleTextures();
    }
}

void PixelStream::updateVisibleTextures()
{
    if (frameBuffer_.empty())
        return;

    PixelStreamSegments &segments = frameBuffer_.front();

    adjustSegmentRendererCount(segments.size());
    assert(segments.size() == segmentRenderers_.size() && "PixelStream::updateVisibleTextures FIXME");

    for(size_t i=0; i<segments.size(); i++)
    {
        if (segmentRenderers_[i]->textureNeedsUpdate() && isVisible(segments[i]))
        {
            if (!segments[i].parameters.compressed)
            {
                const QImage textureWrapper((const uchar*)segments[i].imageData.constData(),
                                            segments[i].parameters.width,
                                            segments[i].parameters.height,
                                            QImage::Format_RGB32);

                segmentRenderers_[i]->updateTexture(textureWrapper);
                segmentRenderers_[i]->setParameters(segments[i].parameters.x, segments[i].parameters.y);
            }
        }
    }
}

void PixelStream::nextFrame()
{
    assert(!frameBuffer_.empty());

    recomputeDimensions(frameBuffer_.front());

    frameBuffer_.pop();

    // Mark all textures as out of date
    for(size_t i=0; i<segmentRenderers_.size(); i++)
    {
        segmentRenderers_[i]->setTextureNeedsUpdate();
    }
}

void PixelStream::recomputeDimensions(const PixelStreamSegments &segments)
{
    width_ = 0;
    height_ = 0;

    for(size_t i=0; i<segments.size(); i++)
    {
        const PixelStreamSegmentParameters& params = segments[i].parameters;
        width_ = std::max(width_, params.width+params.x);
        height_ = std::max(height_, params.height+params.y);
    }
}

void PixelStream::decodeVisibleTextures()
{
    if (frameBuffer_.empty())
        return;

    PixelStreamSegments &segments = frameBuffer_.front();
    adjustFrameDecodersCount(segments.size());

    assert(frameDecoders_.size() == segments.size() && "PixelStream::startDecodingNextFrame FIXME");

    std::vector<PixelStreamSegmentDecoderPtr>::iterator frameDecoder_it = frameDecoders_.begin();
    PixelStreamSegments::iterator segment_it = segments.begin();
    for ( ; segment_it != segments.end(); ++segment_it, ++frameDecoder_it )
    {
        if ( segment_it->parameters.compressed && isVisible(*segment_it) )
        {
            (*frameDecoder_it)->startDecoding(*segment_it);
        }
    }
}

void PixelStream::render(const float tX, const float tY, const float tW, const float tH)
{
    updateRenderedFrameCount();

    const bool showSegmentBorders = g_displayGroupManager->getOptions()->getShowStreamingSegments();
    const bool showSegmentStatistics = g_displayGroupManager->getOptions()->getShowStreamingStatistics();

    glPushMatrix();
    glScalef(1.f/(float)width_, 1.f/(float)height_, 0.f);

    for(std::vector<PixelStreamSegmentRendererPtr>::iterator it=segmentRenderers_.begin(); it != segmentRenderers_.end(); it++)
    {
        if (isVisible( (*it)->getRect( )))
        {
            (*it)->render(showSegmentBorders, showSegmentStatistics);
        }
    }

    glPopMatrix();
}

void PixelStream::adjustFrameDecodersCount(const size_t count)
{
    // We need to insert NEW objects in the vector if it is smaller
    for (size_t i=frameDecoders_.size(); i<count; ++i)
        frameDecoders_.push_back( PixelStreamSegmentDecoderPtr(new PixelStreamSegmentDecoder()) );
    // Or resize it if it is bigger
    frameDecoders_.resize( count );
}

void PixelStream::adjustSegmentRendererCount(const size_t count)
{
    // Recreate the renderers if the number of segments has changed
    if (segmentRenderers_.size() != count)
    {
        segmentRenderers_.clear();
        for (size_t i=0; i<count; ++i)
            segmentRenderers_.push_back( PixelStreamSegmentRendererPtr(new PixelStreamSegmentRenderer(uri_)) );
    }
}

void PixelStream::insertNewFrame(const PixelStreamSegments &segments)
{
    if (frameBuffer_.size() <= MAX_FRAME_QUEUE_SIZE)
        frameBuffer_.push(segments);
}


bool PixelStream::isDecodingInProgress()
{
    // determine if threads are running on any processes for this PixelStream

    // first, for this local process
    int localThreadsRunning = 0;

    std::vector<PixelStreamSegmentDecoderPtr>::const_iterator it;
    for (it = frameDecoders_.begin(); it != frameDecoders_.end(); it++)
    {
        if ((*it)->isRunning())
            ++localThreadsRunning;
    }

    // now, globally for all render processes
    int globalThreadsRunning;

    MPI_Allreduce((void *)&localThreadsRunning, (void *)&globalThreadsRunning, 1, MPI_INT, MPI_SUM, g_mpiRenderComm);

    return (globalThreadsRunning > 0);
}

bool PixelStream::isVisible(const QRectF& segment)
{
    ContentWindowManagerPtr cwm = g_displayGroupManager->getContentWindowManager(uri_, CONTENT_TYPE_PIXEL_STREAM);

    if(cwm)
    {
        const QRectF& window = cwm->getCoordinates();

        // coordinates of segment in global tiled display space
        const double segmentX = window.x() + (double)segment.x() / (double)width_ * window.width();
        const double segmentY = window.y() + (double)segment.y() / (double)height_ * window.height();
        const double segmentW = (double)segment.width() / (double)width_ * window.width();
        const double segmentH = (double)segment.height() / (double)height_ * window.height();

        return g_mainWindow->isRegionVisible(segmentX, segmentY, segmentW, segmentH);
    }
    else
    {
        put_flog(LOG_WARN, "could not find window for segment");
        return false;
    }
}

bool PixelStream::isVisible(const dc::PixelStreamSegment& segment)
{
    ContentWindowManagerPtr cwm = g_displayGroupManager->getContentWindowManager(uri_, CONTENT_TYPE_PIXEL_STREAM);

    if(cwm)
    {
        const dc::PixelStreamSegmentParameters& segmentParams = segment.parameters;
        const QRectF& window = cwm->getCoordinates();

        // coordinates of segment in global tiled display space
        double segmentX = window.x() + (double)segmentParams.x / (double)width_ * window.width();
        double segmentY = window.y() + (double)segmentParams.y / (double)height_ * window.height();
        double segmentW = (double)segmentParams.width / (double)width_ * window.width();
        double segmentH = (double)segmentParams.height / (double)height_ * window.height();

        return g_mainWindow->isRegionVisible(segmentX, segmentY, segmentW, segmentH);
    }
    else
    {
        put_flog(LOG_WARN, "could not find window for segment");
        return false;
    }
}

