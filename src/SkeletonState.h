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

#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include "SkeletonSensor.h"

class ContentWindowInterface;
class DisplayGroupJoystick;

// SkeletonState: keeps track of the current state of the tracked user
// the skeleton can have 3 states:
// free movement: cursors are drawn, but no windows are active
// active window: the active window is moved with the hand and resized with two hands
// focused interaction: hand movement pans and zooms the content of active window
class SkeletonState
{
    public:
        SkeletonState();
        ~SkeletonState(){};

        int update(SkeletonRepresentation& skel);
        void setInactive() { hasControl_ = FALSE; }
        void zoom(SkeletonPoint& lhand, SkeletonPoint& rhand, float threshold);
        void pan(SkeletonPoint& rh, SkeletonPoint& rs, float maxReach);
        void scaleWindow(SkeletonPoint& lhand, SkeletonPoint& rhand, float threshold);
        void render();
        void drawJoints();
        void drawLimb(SkeletonPoint& p1, SkeletonPoint& p2);

        // are hands exceeding depth threshold
        bool leftHandActive_, rightHandActive_;

        // are we interacting with a focused window?
        bool focusInteraction_;

        // the current point representation of the skeleton
        SkeletonRepresentation skeletonRep_;

        // are we the controlling user?
        bool hasControl_;

    protected:
            friend class boost::serialization::access;

            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & leftHandActive_;
                ar & rightHandActive_;
                ar & focusInteraction_;
                ar & skeletonRep_;
                ar & hasControl_;
            }

    private:

        // do we have an active window?
        bool active_;

        // deadCursor: no movement has been detected
        bool deadCursor_;

        // time spend hovering over inactive window
        QTime hoverTime_;

        // timeout for focuse gesture
        QTime focusTimeOut_;

        // timeout for gaining control gesture
        QTime controllerTimeOut_;

        // dead cursor timeout
        QTime deadCursorTimeOut_;

        // window either being hovered over or active
        boost::shared_ptr<ContentWindowInterface> activeWindow_;

        // displayGroup for this skeleton
        boost::shared_ptr<DisplayGroupJoystick> displayGroup_;

};

#endif