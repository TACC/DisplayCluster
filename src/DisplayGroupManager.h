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

#include "main.h"

#include "MessageHeader.h"
#include "DisplayGroupInterface.h"
#include "Options.h"
#include "Marker.h"
#include "config.h"
#include <QtWidgets>
#include <vector>
#include <stack>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#if ENABLE_SKELETON_SUPPORT
    #include "SkeletonState.h"
#endif

class ContentWindowManager;

class DisplayGroupManager : public DisplayGroupInterface, public boost::enable_shared_from_this<DisplayGroupManager> {
    Q_OBJECT

    public:

        DisplayGroupManager();

        boost::shared_ptr<Options> getOptions();

        boost::shared_ptr<Marker> getNewMarker();
        std::vector<boost::shared_ptr<Marker> > getMarkers();

        boost::shared_ptr<boost::posix_time::ptime> getTimestamp();

#if ENABLE_SKELETON_SUPPORT
        std::vector< boost::shared_ptr<SkeletonState> > getSkeletons();
#endif

        // re-implemented DisplayGroupInterface slots
        void addContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void removeContentWindowManager(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);
        void moveContentWindowManagerToFront(boost::shared_ptr<ContentWindowManager> contentWindowManager, DisplayGroupInterface * source=NULL);

				std::stack<QString> state_stack;

				void pushState();
				void popState();

        void receiveMessage();

    public slots:

        bool saveStateXMLFile(std::string filename);
        bool loadStateXMLFile(std::string filename);

				bool loadStateXML(QString);
				bool saveStateXML(QString&);


        void sendDisplayGroup();
        void sendContentsDimensionsRequest();
        void sendPixelStreams();
        void sendParallelPixelStreams();
        void sendSVGStreams();
        void sendQuit();

				void suspendSynchronization();
				void resumeSynchronization();

        void advanceContents();

#if ENABLE_SKELETON_SUPPORT
        void setSkeletons(std::vector<boost::shared_ptr<SkeletonState> > skeletons);
#endif

    private:
				bool synchronization_suspended;

        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int)
        {
            ar & options_;
            ar & markers_;
            ar & contentWindowManagers_;

#if ENABLE_SKELETON_SUPPORT
            ar & skeletons_;
#endif
        }

        // options
        boost::shared_ptr<Options> options_;

        // marker and mutex
        QMutex markersMutex_;
        std::vector<boost::shared_ptr<Marker> > markers_;

        // frame timing
        boost::shared_ptr<boost::posix_time::ptime> timestamp_;

#if ENABLE_SKELETON_SUPPORT
        std::vector<boost::shared_ptr<SkeletonState> > skeletons_;
#endif

        // rank 1 - rank 0 timestamp offset
        boost::posix_time::time_duration timestampOffset_;

        void sendMessage(MESSAGE_TYPE, std::string, char *, int);

        void receiveDisplayGroup(MessageHeader messageHeader, char *);
        void receiveContentsDimensionsRequest(MessageHeader messageHeader, char *);
        void receivePixelStreams(MessageHeader messageHeader, char *);
        void receiveParallelPixelStreams(MessageHeader messageHeader, char *);
        void receiveSVGStreams(MessageHeader messageHeader, char *);
        void receiveFrameClockUpdate(MessageHeader messageHeader, char *);
};

#endif
