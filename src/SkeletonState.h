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

#ifndef SKELETON_STATE_H
#define SKELETON_STATE_H


// timeout (milliseconds) for a missing marker to cause the window to become inactive
#define DEAD_MARKER_TIME 500

// timeout (milliseconds) for a hovering marker to cause the window to become active
#define HOVER_TIME 2000

// time required (milliseconds) between changing interaction modes
#define MODE_CHANGE_TIME 2000

// scale factor for zooming in window
#define WINDOW_ZOOM_FACTOR 0.05

// scale factor for panning in window
#define WINDOW_PAN_FACTOR 0.025

// scale factor for window size scaling
#define WINDOW_SCALE_FACTOR 0.05

#include "SkeletonSensor.h"
#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

class ContentWindowInterface;
class DisplayGroupJoystick;

// SkeletonState: keeps track of the current state of the tracked user
class SkeletonState
{
    public:
        SkeletonState();

        int update(Skeleton skeleton);
        void render();

        // get / set controlling status
        bool getControl();
        void setControl(bool control);

    protected:
            friend class boost::serialization::access;

            template<class Archive>
            void serialize(Archive & ar, const unsigned int)
            {
                ar & focusInteraction_;
                ar & skeleton_;
                ar & control_;
                ar & leftHandActive_;
                ar & rightHandActive_;
            }

    private:

        // display group interface for this skeleton
        boost::shared_ptr<DisplayGroupJoystick> displayGroup_;

        // are we interacting with a focused window?
        bool focusInteraction_;

        // the current point representation of the skeleton
        Skeleton skeleton_;

        // control status and timer
        bool control_;
        QTime controlTimeOut_;

        // are hands exceeding depth threshold
        bool leftHandActive_, rightHandActive_;

        // marker timeout
        QTime markerTimeOut_;

        // time spend hovering over a window
        QTime hoverTime_;
        boost::shared_ptr<ContentWindowInterface> hoverWindow_;

        // if we have an active window or not
        bool activeWindow_;

        // timeout for changing modes
        QTime modeChangeTimeOut_;

        void renderJoints();
        void renderLimb(SkeletonPoint& p1, SkeletonPoint& p2);
};

#endif