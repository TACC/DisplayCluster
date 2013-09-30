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

#ifndef DISPLAY_GROUP_MANAGER_H
#define DISPLAY_GROUP_MANAGER_H

#include "MessageHeader.h"
#include "DisplayGroupInterface.h"
#include "Options.h"
#include "Marker.h"
#include "config.h"
#include "Factory.hpp"
#include <QtGui>
#include <vector>
#ifndef Q_MOC_RUN
// https://bugreports.qt.nokia.com/browse/QTBUG-22829: When Qt moc runs on CGAL
// files, do not process <boost/type_traits/has_operator.hpp>
#  include <boost/shared_ptr.hpp>
#  include <boost/enable_shared_from_this.hpp>
#  include <boost/date_time/posix_time/posix_time.hpp>
#endif

#if ENABLE_SKELETON_SUPPORT
    #include "SkeletonState.h"
#endif

#include "PixelStream.h"
#include "serializationHelpers.h"


class ContentWindowManager;

class DisplayGroupManager : public DisplayGroupInterface, public boost::enable_shared_from_this<DisplayGroupManager> {
    Q_OBJECT

    public:

        DisplayGroupManager();

        boost::shared_ptr<Options> getOptions() const;

        boost::shared_ptr<Marker> getNewMarker();
        const std::vector<boost::shared_ptr<Marker> >& getMarkers() const;

        boost::posix_time::ptime getTimestamp() const;

#if ENABLE_SKELETON_SUPPORT
        std::vector< boost::shared_ptr<SkeletonState> > getSkeletons();
#endif

        // re-implemented DisplayGroupInterface slots
        void addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);

        // find the offset between the rank 0 clock and the rank 1 clock. recall the rank 1 clock is used across rank 1 - n.
        void calibrateTimestampOffset();

        // background content
        void setBackgroundContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager);
        boost::shared_ptr<ContentWindowManager> getBackgroundContentWindowManager() const;

        // background
        void initBackground();
        QColor getBackgroundColor() const;
        void setBackgroundColor(QColor color);

    public slots:

        // this can be invoked from other threads to construct a DisplayGroupInterface and move it to that thread
        boost::shared_ptr<DisplayGroupInterface> getDisplayGroupInterface(QThread * thread);

        bool saveStateXMLFile(std::string filename);
        bool loadStateXMLFile(std::string filename);

        void receiveMessages();

        void sendDisplayGroup();
        void sendContentsDimensionsRequest();
        void sendPixelStreams();
        void sendPixelStreamSegments(const std::vector<PixelStreamSegment> &segments, const QString& uri);
        void sendSVGStreams();
        void sendFrameClockUpdate();
        void receiveFrameClockUpdate();
        void sendQuit();

        void advanceContents();

#if ENABLE_SKELETON_SUPPORT
        void setSkeletons(std::vector<boost::shared_ptr<SkeletonState> > skeletons);
#endif
        // Rank0 manages pixel stream events
        void processPixelStreamSegment(QString uri, PixelStreamSegment segment);
        void openPixelStream(QString uri, int width, int height);
        void adjustPixelStreamContentDimensions(QString uri, int width, int height, bool changeViewSize);
        void deletePixelStream(QString uri);

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int)
        {
            ar & options_;
            ar & markers_;
            ar & contentWindowManagers_;
            ar & backgroundContent_;
            ar & backgroundColor_;

#if ENABLE_SKELETON_SUPPORT
            ar & skeletons_;
#endif
        }

        // background
        boost::shared_ptr<ContentWindowManager> backgroundContent_;
        QColor backgroundColor_;

        // options
        boost::shared_ptr<Options> options_;

        // marker and mutex
        QMutex markersMutex_;
        std::vector<boost::shared_ptr<Marker> > markers_;

        // frame timing
        boost::posix_time::ptime timestamp_;

#if ENABLE_SKELETON_SUPPORT
        std::vector<boost::shared_ptr<SkeletonState> > skeletons_;
#endif

        // rank 1 - rank 0 timestamp offset
        boost::posix_time::time_duration timestampOffset_;

        // Rank0: Input buffer for PixelStreams
        Factory<PixelStream> pixelStreamSourceFactory_;


        void receiveDisplayGroup(MessageHeader messageHeader);
        void receiveContentsDimensionsRequest(MessageHeader messageHeader);
        void receivePixelStreams(MessageHeader messageHeader);
        void receiveSVGStreams(MessageHeader messageHeader);

    signals:
        // Rank0 signals pixel streams events
        void pixelStreamViewClosed(QString uri);
};

#endif
