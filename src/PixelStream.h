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

#ifndef PIXEL_STREAM_H
#define PIXEL_STREAM_H

#include "FactoryObject.h"
#include "PixelStreamSegment.h"
#include "PixelStreamSegmentParameters.h"
#include "PixelStreamSegmentRenderer.h"
#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/binary_object.hpp>
#include <boost/serialization/split_member.hpp>
#include <string>
#include <map>
#include <vector>

class PixelStream : public FactoryObject {

    public:

        PixelStream(QString uri);

        void getDimensions(int &width, int &height);
        void render(float tX, float tY, float tW, float tH);

        void insertSegment(PixelStreamSegment segment);

        // retrieve latest segments and remove them (and older segments) from the map
        std::vector<PixelStreamSegment> getAndPopLatestSegments();

        // retrieve all segments and clear the map
        std::vector<PixelStreamSegment> getAndPopAllSegments();

        // retrieve all segments for the given frame index and clear older entries in the map
        std::vector<PixelStreamSegment> getAndPopSegments(int frameIndex);

        // update renderers to the latest segments
        void updateSegmentRenderers();

        // Has sender requested the view dimensions to be changed
        bool changeViewDimensionsRequested();

    private:

        // pixel stream identifier
        QString uri_;

        // dimensions of entire pixel stream
        int width_;
        int height_;

        // Number of segments
        int segmentCount_;

        // Sender requests the view dimensions to be changed
        bool changeViewDimensionsRequested_;

        // segments mutex
        QMutex segmentsMutex_;

        // for each source, vector of pixel stream segments
        // use a vector here since it may allow for easier frame synchronization later
        std::map<int, std::vector<PixelStreamSegment> > segments_;

        // for each source, pixel stream object for image decoding and parameters
        std::map<int, boost::shared_ptr<PixelStreamSegmentRenderer> > segmentRenderers_;
        std::map<int, PixelStreamSegmentParameters> pixelStreamParameters_;

        // determine if segment is visible on any of the screens of this process
        bool isSegmentVisible(PixelStreamSegmentParameters parameters);

        // get vector of source indices visible on any of the screens of this process
        std::vector<int> getSourceIndicesVisible();

        // get whether or not we have valid frame indices for all segments
        bool getValidFrameIndices();

        // clear old / stale pixel streams from map
        //void clearStalePixelStreams();

        // statistics
        std::map<int, std::vector<QTime> > segmentsRenderTimes_;

        void updateStatistics(int sourceIndex);
        std::string getStatistics(int sourceIndex);

        // Global streaming synchronization helper methods
        int getGlobalLoadImageDataThreadsRunning();
        int getGlobalLatestVisibleFrameIndex();
};


#endif
