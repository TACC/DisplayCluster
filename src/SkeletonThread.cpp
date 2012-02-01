#include "SkeletonThread.h"
#include "SkeletonSensor.h"
#include "DisplayGroupJoystick.h"
#include "ContentWindowInterface.h"
#include "main.h"
#include "log.h"

// set the timer to ping for new sensor data every 10 milliseconds
const unsigned int SKELETON_TIMER_INTERVAL = 1000/60;

SkeletonThread::SkeletonThread()
{
    connect(this, SIGNAL(skeletonsUpdated(std::vector<SkeletonState>)), g_displayGroupManager.get(), SLOT(setSkeletons(std::vector<SkeletonState>)), Qt::QueuedConnection);
}

SkeletonThread::~SkeletonThread()
{
    delete sensor_;
}

std::vector<SkeletonState> SkeletonThread::getSkeletons()
{

    std::vector<SkeletonState> skeletons;

    std::map<unsigned int, SkeletonState>::iterator it;

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

    /*
     *
     * This code is for tracking only the closest user
     * ===============================================
    // there are no users to be tracked
    if (!sensor_->isTracking())
    {
        states_.clear();
        return;
    }

    // get the closest user
    unsigned int closestUID = sensor_->getClosestTrackedUID();

    // if user was not tracked at last frame create new state
    if (states_.count(closestUID) == 0)
    {
        SkeletonRepresentation skeletonRep = sensor_->getAllAvailablePoints(closestUID);
        SkeletonState state = SkeletonState();
        state.update(skeletonRep);
        states_.insert(std::pair<unsigned int, SkeletonState>(closestUID, state));
    }
    else
    {
        SkeletonRepresentation skeletonRep = sensor_->getAllAvailablePoints(closestUID);
        states_[closestUID].update(skeletonRep);
    }
    */

    // update number of tracked users
    if (!sensor_->isTracking())
    {
        states_.clear();
        return;
    }

    bool newActive = FALSE; // we use this to keep track if there is a new active user
    unsigned int activeUID;

    for (unsigned int i = 0; i < sensor_->getNOTrackedUsers(); i++)
    {
        // check if skeletonState exists for user and create if not
        if (states_.count(sensor_->getUID(i)) == 0)
        {
            SkeletonRepresentation skeletonRep = sensor_->getAllAvailablePoints(sensor_->getUID(i));
            SkeletonState state = SkeletonState();
            state.update(skeletonRep);
            states_.insert(std::pair<unsigned int, SkeletonState>(sensor_->getUID(i), state));
        }
        else
        {
            SkeletonRepresentation skeletonRep = sensor_->getAllAvailablePoints(sensor_->getUID(i));

            // if this skeleton is active, save its index for later so we can set all others inactive
            if (states_[sensor_->getUID(i)].update(skeletonRep) == 1)
            {
                newActive = TRUE;
                activeUID = sensor_->getUID(i);
            }
        }
    }

    // iterate through users, marking all but newly active, inactive
    std::map<unsigned int, SkeletonState>::iterator it;
    if(newActive)
    {
        for(it = states_.begin(); it != states_.end(); it++)
        {
            if((*it).first != activeUID)
                (*it).second.setInactive();
        }
    }

    // remove all untracked users and set the states correctly
    for(it = states_.begin(); it != states_.end(); it++)
    {
        unsigned int uid = (*it).first;

        if(!sensor_->isTracking(uid))
        {
            states_.erase(uid);
        }
    }

    emit(skeletonsUpdated(getSkeletons()));
    emit(updateSkeletonsFinished());
}
