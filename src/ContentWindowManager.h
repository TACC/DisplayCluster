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

#ifndef CONTENT_WINDOW_MANAGER_H
#define CONTENT_WINDOW_MANAGER_H

#include "ContentWindowInterface.h"
#include "Content.h"

#include <QtGui>
#include <QtWidgets>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/weak_ptr.hpp>
#include <boost/date_time/posix_time/time_serialize.hpp>
#include <boost/serialization/string.hpp>

class DisplayGroupManager;

class ContentWindowManager : public ContentWindowInterface, public boost::enable_shared_from_this<ContentWindowManager> {

    public:

        ContentWindowManager() { } // no-argument constructor required for serialization
        ContentWindowManager(boost::shared_ptr<Content> content);

        boost::shared_ptr<Content> getContent();

        // unique name identifying this window, used by the remote API; empty
        // until the DisplayGroupManager assigns one, in which case the URI stands in
        std::string getName();
        void setName(std::string name);

        // sensor control: a window with a nonzero sensor mask has its visibility
        // set by the wall's sensor value (see DisplayGroupManager::setSensors()).
        // It's visible when any sensor in the mask is lit if showWhenLit, or
        // when none of them is lit if not. A mask of 0 - the default - leaves
        // the window alone.
        uint32_t getSensorMask() { return sensorMask_; }
        bool getShowWhenLit() { return showWhenLit_; }
        void setSensorControl(uint32_t sensorMask, bool showWhenLit);

        // whether the window should be hidden for the given sensor value; only
        // meaningful when the sensor mask is nonzero
        bool hiddenForSensors(uint32_t sensors);

        boost::shared_ptr<DisplayGroupManager> getDisplayGroupManager();
        void setDisplayGroupManager(boost::shared_ptr<DisplayGroupManager> displayGroupManager);

        // re-implemented ContentWindowInterface slots
        void moveToFront(ContentWindowInterface * source=NULL);
        void close(ContentWindowInterface * source=NULL);

        // GLWindow rendering
        void render();

    protected:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int)
        {
            ar & content_;
            ar & name_;
            ar & displayGroupManager_;
            ar & contentWidth_;
            ar & contentHeight_;
            ar & x_;
            ar & y_;
            ar & w_;
            ar & h_;
            ar & centerX_;
            ar & centerY_;
            ar & zoom_;
            ar & selected_;
            ar & hidden_;
            ar & highlightedTimestamp_;
        }

    private:

        boost::shared_ptr<Content> content_;

        std::string name_;

        // only used on the master process, so not serialized
        uint32_t sensorMask_ = 0;
        bool showWhenLit_ = true;

        boost::weak_ptr<DisplayGroupManager> displayGroupManager_;
};

#endif
