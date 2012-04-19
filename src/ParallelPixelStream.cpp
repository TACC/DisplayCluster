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
#include "main.h"

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

    QMutexLocker locker(&segmentsMutex_);

    // iterate through map, rendering the latest content available from each source
    // later, this could be synchronized...
    // for each source
    for(std::map<int, std::vector<ParallelPixelStreamSegment> >::iterator  it=segments_.begin(); it != segments_.end(); it++)
    {
        // for each frame
        for(int i=(int)(*it).second.size() - 1; i>=0; i--)
        {
            // OpenGL transformation
            glPushMatrix();

            float x = (float)((*it).second[i].parameters.x) / (float)width_;
            float y = (float)((*it).second[i].parameters.y) / (float)height_;
            float width = (float)((*it).second[i].parameters.width) / (float)width_;
            float height = (float)((*it).second[i].parameters.height) / (float)height_;

            glTranslatef(x, y, 0.);
            glScalef(width, height, 0.);

            // todo: compute actual texture bounds to render considering zoom, pan

            bool rendered = false;

            if((*it).second[i].pixelStream->render(0.,0.,1.,1.) == true)
            {
                // successful render of texture
                // delete older segments and break
                (*it).second.erase((*it).second.begin(), (*it).second.begin() + i);

                rendered = true;
            }

            glPopMatrix();

            if(rendered == true)
            {
                break;
            }
        }
    }
}

void ParallelPixelStream::insertSegment(ParallelPixelStreamSegment segment)
{
    // todo: filter segment insertions based on visibility

    QMutexLocker locker(&segmentsMutex_);

    // update total dimensions
    width_ = segment.parameters.totalWidth;
    height_ = segment.parameters.totalHeight;

    // create pixel stream object and give it image data (only for rank != 0)
    // this will trigger the image loading thread to run
    // URI doesn't matter here...
    if(g_mpiRank != 0)
    {
        boost::shared_ptr<PixelStream> ps(new PixelStream("ParallelPixelStreamSegment"));
        ps->setImageData(segment.imageData);

        segment.pixelStream = ps;
    }

    segments_[(int)segment.parameters.sourceIndex].push_back(segment);
}

std::vector<ParallelPixelStreamSegment> ParallelPixelStream::getAndPopLatestSegments()
{
    QMutexLocker locker(&segmentsMutex_);

    std::vector<ParallelPixelStreamSegment> latestSegments;

    for(std::map<int, std::vector<ParallelPixelStreamSegment> >::iterator it=segments_.begin(); it != segments_.end(); it++)
    {
        latestSegments.push_back((*it).second.back());
    }

    // clear the map since we got all the latest segments
    segments_.clear();

    return latestSegments;
}

Factory<ParallelPixelStream> g_parallelPixelStreamSourceFactory;
