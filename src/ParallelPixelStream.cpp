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

#include "ParallelPixelStream.h"
#include "ContentWindowManager.h"
#include "MainWindow.h"
#include "DisplayGroupManager.h"
#include "log.h"

ParallelPixelStream::ParallelPixelStream(std::string uri)
{
    // defaults
    width_ = 0;
    height_ = 0;

    // assign values
    uri_ = uri;
}

void ParallelPixelStream::getDimensions(int &width, int &height)
{
    QMutexLocker locker(&segmentsMutex_);

    width = width_;
    height = height_;
}

void ParallelPixelStream::render(float tX, float tY, float tW, float tH)
{
    updateRenderedFrameCount();

    for(std::map<int, boost::shared_ptr<PixelStream> >::iterator it=pixelStreams_.begin(); it != pixelStreams_.end(); it++)
    {
        int sourceIndex = (*it).first;
        boost::shared_ptr<PixelStream> pixelStream = (*it).second;

        // skip segments not visible
        if(isSegmentVisible(pixelStreamParameters_[sourceIndex]) == false)
        {
            // don't render, continue to next segment
            continue;
        }

        // OpenGL transformation
        glPushMatrix();

        float x = (float)(pixelStreamParameters_[sourceIndex].x) / (float)width_;
        float y = (float)(pixelStreamParameters_[sourceIndex].y) / (float)height_;
        float width = (float)(pixelStreamParameters_[sourceIndex].width) / (float)width_;
        float height = (float)(pixelStreamParameters_[sourceIndex].height) / (float)height_;

        glTranslatef(x, y, 0.);
        glScalef(width, height, 0.);

        // todo: compute actual texture bounds to render considering zoom, pan

        pixelStream->render(0.,0.,1.,1.);

        bool showStreamingSegments = g_displayGroupManager->getOptions()->getShowStreamingSegments();
        bool showStreamingStatistics = g_displayGroupManager->getOptions()->getShowStreamingStatistics();

        if(showStreamingSegments == true || showStreamingStatistics == true)
        {
            glPushAttrib(GL_CURRENT_BIT | GL_LINE_BIT | GL_DEPTH_BUFFER_BIT);
            glLineWidth(2);

            glPushMatrix();
            glTranslatef(0.,0.,0.05);

            // render segment borders
            if(showStreamingSegments == true)
            {
                glColor4f(1.,1.,1.,1.);

                glBegin(GL_LINE_LOOP);

                glVertex2f(0.,0.);
                glVertex2f(1.,0.);
                glVertex2f(1.,1.);
                glVertex2f(0.,1.);

                glEnd();
            }

            // render segment statistics
            if(showStreamingStatistics == true)
            {
                // render statistics
                std::string statisticsString = getStatistics((*it).first);

                QFont font;
                font.setPixelSize(48);

                glColor4f(1.,0.,0.,1.);
                glDisable(GL_DEPTH_TEST);
                g_mainWindow->getActiveGLWindow()->renderText(0.1, 0.95, 0., QString(statisticsString.c_str()), font);
            }

            glPopMatrix();
            glPopAttrib();
        }

        glPopMatrix();
    }

    // get rid of old / stale pixel streams
    clearStalePixelStreams();
}

void ParallelPixelStream::insertSegment(ParallelPixelStreamSegment segment)
{
    QMutexLocker locker(&segmentsMutex_);

    // update total dimensions if we have non-blank parameters
    if(segment.parameters.totalWidth != 0 && segment.parameters.totalHeight != 0)
    {
        width_ = segment.parameters.totalWidth;
        height_ = segment.parameters.totalHeight;
    }

    // update parameters
    pixelStreamParameters_[segment.parameters.sourceIndex] = segment.parameters;

    // delete any blank segments
    // filter out segments that are not visible (only for rank != 0)
    if(g_mpiRank != 0)
    {
        if(segment.parameters.totalWidth == 0 && segment.parameters.totalHeight == 0)
        {
            // this is a blank segment, clear out everything for its sourceIndex...
            segments_.erase(segment.parameters.sourceIndex);
            pixelStreams_.erase(segment.parameters.sourceIndex);
            pixelStreamParameters_.erase(segment.parameters.sourceIndex);

            // drop the segment
            return;
        }
        else if(isSegmentVisible(segment.parameters) == false)
        {
            // clear any unprocessed segments for this source index
            if(segments_.count(segment.parameters.sourceIndex) != 0)
            {
                segments_.erase(segment.parameters.sourceIndex);
            }

            // drop the segment
            return;
        }
    }

    segments_[(int)segment.parameters.sourceIndex].push_back(segment);
}

std::vector<ParallelPixelStreamSegment> ParallelPixelStream::getAndPopLatestSegments()
{
    QMutexLocker locker(&segmentsMutex_);

    std::vector<ParallelPixelStreamSegment> latestSegments;

    for(std::map<int, std::vector<ParallelPixelStreamSegment> >::iterator it=segments_.begin(); it != segments_.end(); it++)
    {
        if((*it).second.size() > 0)
        {
            latestSegments.push_back((*it).second.back());
        }
    }

    // clear the map since we got all the latest segments
    segments_.clear();

    return latestSegments;
}

std::vector<ParallelPixelStreamSegment> ParallelPixelStream::getAndPopAllSegments()
{
    QMutexLocker locker(&segmentsMutex_);

    std::vector<ParallelPixelStreamSegment> allSegments;

    for(std::map<int, std::vector<ParallelPixelStreamSegment> >::iterator it=segments_.begin(); it != segments_.end(); it++)
    {
        if((*it).second.size() > 0)
        {
            allSegments.insert(allSegments.end(), (*it).second.begin(), (*it).second.end());
        }
    }

    // clear the map since we got all the segments
    segments_.clear();

    return allSegments;
}

std::vector<ParallelPixelStreamSegment> ParallelPixelStream::getAndPopSegments(int frameIndex)
{
    QMutexLocker locker(&segmentsMutex_);

    std::vector<ParallelPixelStreamSegment> frameIndexSegments;

    for(std::map<int, std::vector<ParallelPixelStreamSegment> >::iterator it=segments_.begin(); it != segments_.end(); it++)
    {
        for(unsigned int i=0; i<(*it).second.size(); i++)
        {
            if((*it).second[i].parameters.frameIndex == frameIndex)
            {
                frameIndexSegments.push_back((*it).second[i]);

                // erase this segment and the earlier segments (i+1 segments will be erased)
                (*it).second.erase((*it).second.begin(), (*it).second.begin() + i+1);

                // continue to next source index in the map (breaking from this for loop)
                break;
            }
        }
    }

    return frameIndexSegments;
}

void ParallelPixelStream::updatePixelStreams()
{
    // segments we want to process
    std::vector<ParallelPixelStreamSegment> segments;

    // process differently depending on synchronization option
    bool enableStreamingSynchronization = g_displayGroupManager->getOptions()->getEnableStreamingSynchronization();

    // make sure all of our segments have a valid frame index
    // if this is not the case, then we can't have synchronization
    if(enableStreamingSynchronization == true && getValidFrameIndices() != true)
    {
        enableStreamingSynchronization = false;
    }

    if(enableStreamingSynchronization == true)
    {
        // determine if threads are running on any processes for this ParallelPixelStream

        // first, for this local process
        int localThreadsRunning = 0;

        std::map<int, boost::shared_ptr<PixelStream> >::iterator it = pixelStreams_.begin();

        while(it != pixelStreams_.end())
        {
            localThreadsRunning += (int)(*it).second->getLoadImageDataThreadRunning();
            it++;
        }

        // now, globally for all render processes
        int globalThreadsRunning;

        MPI_Allreduce((void *)&localThreadsRunning, (void *)&globalThreadsRunning, 1, MPI_INT, MPI_SUM, g_mpiRenderComm);

        // do nothing if threads are still running
        if(globalThreadsRunning > 0)
        {
            return;
        }

        // if no threads are running, attempt to update textures (this will be synchronous across all streams!)
        it = pixelStreams_.begin();

        while(it != pixelStreams_.end())
        {
            (*it).second->updateTextureIfAvailable();
            it++;
        }

        // find the latest frame index we have locally for all visible parameters

        // the visible source indices
        std::vector<int> visibleSourceIndices = getSourceIndicesVisible();

        // the latest frame index we have for all visible source indices
        int latestFrameIndex = INT_MAX;

        for(unsigned int i=0; i<visibleSourceIndices.size(); i++)
        {
            if(segments_.count(visibleSourceIndices[i]) == 0 || segments_[visibleSourceIndices[i]].size() == 0)
            {
                latestFrameIndex = -1;
            }
            else
            {
                latestFrameIndex = std::min(latestFrameIndex, segments_[visibleSourceIndices[i]].back().parameters.frameIndex);
            }
        }

        // now, find the latest frame index for visible source indices across all nodes
        int globalLatestFrameIndex;

        MPI_Allreduce((void *)&latestFrameIndex, (void *)&globalLatestFrameIndex, 1, MPI_INT, MPI_MIN, g_mpiRenderComm);

        if(globalLatestFrameIndex > 0 && globalLatestFrameIndex != INT_MAX)
        {
            segments = getAndPopSegments(globalLatestFrameIndex);
        }
    }
    else
    {
        // synchronization disabled
        segments = getAndPopLatestSegments();
    }

    for(unsigned int i=0; i<segments.size(); i++)
    {
        int sourceIndex = segments[i].parameters.sourceIndex;

        if(pixelStreams_[sourceIndex] == NULL)
        {
            boost::shared_ptr<PixelStream> ps(new PixelStream("ParallelPixelStreamSegment"));
            pixelStreams_[sourceIndex] = ps;
        }

        // auto texture uploading depending on synchronous setting
        pixelStreams_[sourceIndex]->setAutoUpdateTexture(!enableStreamingSynchronization);

        bool success = pixelStreams_[sourceIndex]->setImageData(segments[i].imageData);

        if(success == true)
        {
            frameUpdated(sourceIndex);
        }
    }
}

bool ParallelPixelStream::isSegmentVisible(ParallelPixelStreamSegmentParameters parameters)
{
    boost::shared_ptr<ContentWindowManager> cwm = g_displayGroupManager->getContentWindowManager(uri_, CONTENT_TYPE_PARALLEL_PIXEL_STREAM);

    if(cwm != NULL)
    {
        // todo: also consider zoom / pan (texture coordinates!)

        double x, y, w, h;
        cwm->getCoordinates(x, y, w, h);

        // coordinates of segment in tiled display space
        double segmentX = x + (double)parameters.x / (double)parameters.totalWidth * w;
        double segmentY = y + (double)parameters.y / (double)parameters.totalHeight * h;
        double segmentW = (double)parameters.width / (double)parameters.totalWidth * w;
        double segmentH = (double)parameters.height / (double)parameters.totalHeight * h;

        bool segmentVisible = false;

        std::vector<boost::shared_ptr<GLWindow> > glWindows = g_mainWindow->getGLWindows();

        for(unsigned int i=0; i<glWindows.size(); i++)
        {
            if(glWindows[i]->isScreenRectangleVisible(segmentX, segmentY, segmentW, segmentH) == true)
            {
                segmentVisible = true;
                break;
            }
        }

        return segmentVisible;
    }
    else
    {
        // return true if we can't find a window
        put_flog(LOG_WARN, "could not find window for segment");

        return true;
    }
}

std::vector<int> ParallelPixelStream::getSourceIndicesVisible()
{
    std::vector<int> sourceIndices;

    std::map<int, ParallelPixelStreamSegmentParameters>::iterator it = pixelStreamParameters_.begin();

    while(it != pixelStreamParameters_.end())
    {
        if(isSegmentVisible((*it).second) == true)
        {
            sourceIndices.push_back((*it).first);
        }

        it++;
    }

    return sourceIndices;
}

bool ParallelPixelStream::getValidFrameIndices()
{
    std::map<int, ParallelPixelStreamSegmentParameters>::iterator it = pixelStreamParameters_.begin();

    while(it != pixelStreamParameters_.end())
    {
        if((*it).second.frameIndex == FRAME_INDEX_UNDEFINED)
        {
            return false;
        }

        it++;
    }

    return true;
}

void ParallelPixelStream::clearStalePixelStreams()
{
    std::map<int, boost::shared_ptr<PixelStream> >::iterator it = pixelStreams_.begin();

    while(it != pixelStreams_.end())
    {
        boost::shared_ptr<PixelStream> pixelStream = (*it).second;

        if(g_frameCount - pixelStream->getRenderedFrameCount() > 1)
        {
            put_flog(LOG_DEBUG, "erasing stale pixel stream");
            pixelStreams_.erase(it++);  // note the post increment; increments the iterator but returns original value for erase
        }
        else
        {
            it++;
        }
    }
}

void ParallelPixelStream::frameUpdated(int sourceIndex)
{
    static unsigned int numHistory = 30;

    segmentsRenderTimes_[sourceIndex].push_back(QTime::currentTime());

    // see if we need to remove an entry
    while(segmentsRenderTimes_[sourceIndex].size() > numHistory)
    {
        segmentsRenderTimes_[sourceIndex].erase(segmentsRenderTimes_[sourceIndex].begin());
    }
}

std::string ParallelPixelStream::getStatistics(int sourceIndex)
{
    QString result;

    if(segmentsRenderTimes_[sourceIndex].size() > 0)
    {
        float fps = (float)segmentsRenderTimes_[sourceIndex].size() / (float)segmentsRenderTimes_[sourceIndex].front().msecsTo(segmentsRenderTimes_[sourceIndex].back()) * 1000.;

        result += QString::number(fps, 'g', 4);
        result += " fps";
    }

    return result.toStdString();
}

Factory<ParallelPixelStream> g_parallelPixelStreamSourceFactory;
