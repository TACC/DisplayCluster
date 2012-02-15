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

#include "SkeletonThread.h"
#include "SkeletonSensor.h"
#include "DisplayGroupJoystick.h"
#include "ContentWindowInterface.h"
#include "main.h"
#include "log.h"

// set the timer to ping for new sensor data every 10 milliseconds
const unsigned int SKELETON_TIMER_INTERVAL = 1000/60;

// an identifier for a UID that is not applicable
const unsigned int NA_UID = 9999;

SkeletonThread::SkeletonThread()
{
    connect(this, SIGNAL(skeletonsUpdated(std::vector< boost::shared_ptr<SkeletonState> >)), g_displayGroupManager.get(), SLOT(setSkeletons(std::vector< boost::shared_ptr<SkeletonState> >)), Qt::QueuedConnection);
}

SkeletonThread::~SkeletonThread()
{
    delete sensor_;
}

std::vector< boost::shared_ptr<SkeletonState> > SkeletonThread::getSkeletons()
{
    std::vector< boost::shared_ptr<SkeletonState> > skeletons;

    std::map<unsigned int, boost::shared_ptr<SkeletonState> >::iterator it;

    for(it=states_.begin(); it != states_.end(); it++)
    {   
        skeletons.push_back((*it).second);
    }

    return skeletons;
}

void SkeletonThread::run()
{

    // Initialize the OpenNI sensor and start generating depth data
    sensor_ = new SkeletonSensor();

    if(sensor_->initialize() != 1)
    {
        put_flog(LOG_ERROR, "Could not initialize skeleton sensor subsystem.");
        return;
    }

    put_flog(LOG_DEBUG, "SkeletonSensor initialized.");

    sensor_->setCalibrationPoseCallbacks();

    // setup timer to repeatedly update
    connect(&timer_, SIGNAL(timeout()), this, SLOT(updateSkeletons()));
    timer_.start(SKELETON_TIMER_INTERVAL);

    exec();
}

void SkeletonThread::updateSkeletons()
{
    // wait for new data from sensor
    sensor_->waitForDeviceUpdateOnUser();

    // get new list of tracked users
    sensor_->updateTrackedUsers();

    bool newActive = FALSE; // we use this to keep track if there is a new active user
    unsigned int activeUID;
    unsigned int previousActiveUID = NA_UID;

    for (unsigned int i = 0; i < sensor_->getNOTrackedUsers(); i++)
    {
        // check if skeletonState exists for user and create if not
        if (states_.count(sensor_->getUID(i)) == 0)
        {
            Skeleton skeleton = sensor_->getAllAvailablePoints(sensor_->getUID(i));
            boost::shared_ptr<SkeletonState> state(new SkeletonState());

            state->update(skeleton);
            states_.insert(std::pair<unsigned int, boost::shared_ptr<SkeletonState> >(sensor_->getUID(i), state));
        }
        else
        {
            // determine if this skeleton has control currently
            if (states_[sensor_->getUID(i)]->hasControl_)
                previousActiveUID = sensor_->getUID(i);
                
            Skeleton skeleton = sensor_->getAllAvailablePoints(sensor_->getUID(i));

            // if this skeleton is active, save its index for later so we can set all others inactive
            if (states_[sensor_->getUID(i)]->update(skeleton) == 1)
            {
                newActive = TRUE;
                activeUID = sensor_->getUID(i);
            }
        }
    }

    // make previous active user inactive if there is a new active user
    if(newActive)
    {
        if (previousActiveUID != NA_UID)
            states_[previousActiveUID]->hasControl_ = FALSE;
    }

    // iterate through skeletons, if the skeleton is not tracked, then delete it
    std::map<unsigned int, boost::shared_ptr<SkeletonState> >::iterator it;
    for(it = states_.begin(); it != states_.end(); it++)
    {
        unsigned int uid = (*it).first;

        if(!sensor_->isTracking(uid))
        {
            states_.erase(uid);
        }
    }

    emit(skeletonsUpdated(getSkeletons()));
}
