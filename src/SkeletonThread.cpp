#include "SkeletonThread.h"
#include "SkeletonSensor.h"
#include "log.h"

// Set the timer to ping for new data every 10 milliseconds
const unsigned int SKELETON_TIMER_INTERVAL = 10;

SkeletonThread::SkeletonThread()
{
    
}

SkeletonThread::~SkeletonThread()
{
    delete sensor_;
}

void SkeletonThread::run()
{

    // Initialize the OpenNI sensor and start generating depth data
    sensor_ = new SkeletonSensor();
    
    if(sensor_->initialize() != 1)
    {
        put_flog(LOG_ERROR, "Could not initialize Skeleton Sensor subsystem.");
        return;
    }
    
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
    
    // Test: print number of tracked users
    if (sensor_->isTracking())
        printf("Currently tracking %d users\n", sensor_->getNOTrackedUsers());
}