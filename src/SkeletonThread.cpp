#include "SkeletonThread.h"
#include "SkeletonSensor.h"
#include "DisplayGroupJoystick.h"
#include "ContentWindowInterface.h"
#include "main.h"
#include "log.h"
#include "vector.h"

// set the timer to ping for new sensor data every 10 milliseconds
const unsigned int SKELETON_TIMER_INTERVAL = 10;
// 2 second hovertime constant
const int HOVER_TIME              = 2000;
// timeout for focus gesture
const int FOCUS_TIME              = 2000;
// timeout for no hand detected
const int DEAD_CURSOR_TIME        = 500;
// scale factor for window size scaling
const float WINDOW_SCALING_FACTOR = 0.05;

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
        put_flog(LOG_ERROR, "Could not initialize skeleton sensor subsystem.");
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
    
    /*
    for (unsigned int i = 0; i < sensor_->getNOTrackedUsers(); i++)
    {
        // for each user, update the user's state
        if(states_.count(sensor_->getUID(i)) == 0)
        {
            SkeletonState state = SkeletonState(sensor_->getAllAvailablePoints(sensor_->getUID(i)));
            state.update(sensor_->getAllAvailablePoints(sensor_->getUID(i)));
            states_.insert(std::pair<unsigned int, SkeletonState>(sensor_->getUID(i), state));
        }
        states_[sensor_->getUID(i)].update(sensor_->getAllAvailablePoints(sensor_->getUID(i)));
    }
    */
    
}

/************************
**SkeletonState methods
*************************/

float calculateDistance(SkeletonPoint& a, SkeletonPoint& b)
{
    float result = sqrt(pow(a.x_ - b.x_, 2) +
                        pow(a.y_ - b.y_, 2) +
                        pow(a.z_ - b.z_, 2));
    return result;
}

SkeletonState::SkeletonState(): active_(FALSE),
                                focusInteraction_(FALSE),
                                deadCursor_(FALSE),
                                scaling_(FALSE),
                                neutralScaleDistance_(0.0),
                                hoverTime_(),
                                focusTimeOut_(),
                                deadCursorTimeOut_(),
                                activeWindow_(),
                                displayGroup_(new DisplayGroupJoystick(g_displayGroupManager))
{
}

void SkeletonState::update(SkeletonRepresentation& skel)
{
    // the process:
    // 1. get hand locations
    // 2. find cursor position by normalizing hand positions to maximum reach
    // 3. draw active cursor if depth threshold exceeded but no activity
    // 4. updated state with new hover time and speed if applicable
    // 5. if not active state and if hover time exceeds threshold, set active and activate window under cursor
    // 6. if active state, check for interacting focus if not already enabled, if pose detected, enable focusing, otherwise update window position and size
    // 7. if interacting focus, process pan or zoom on content, if pose detected, disable active and focus
    
    // depth threshold - using divinci vitruvian man, threshold is 3/16 height of skeleton
    float depthThreshold = 3.0/16.0 * (calculateDistance(skel.head_, skel.neck_)  +
                                       calculateDistance(skel.neck_, skel.torso_) +
                                       calculateDistance(skel.torso_, skel.leftFoot_));
    
    printf("depthThreshold: %f\n", depthThreshold);
    // 1. get normalized hand locations
    
    // magnitude of arm length
    float armLength = calculateDistance(skel.rightHand_, skel.rightElbow_) +
                      calculateDistance(skel.rightElbow_, skel.rightShoulder_);
    
    // the maximum distance that can be reached by arm while crossing depth plane
    float maxReach = sqrt(armLength*armLength + depthThreshold*depthThreshold);
    
    // 2. find cursor position (normalized)
    float normX = ((skel.rightHand_.x_ - skel.rightShoulder_.x_) + maxReach)/(2*maxReach);
    float normY = 1 - ((skel.rightHand_.y_ - skel.rightShoulder_.y_) + maxReach)/(2*maxReach);
    
    printf("normX,Y = %f, %f\n", normX, normY);
    
    // are we confident in relevant joint positions?
    bool confidenceLeft  = FALSE;
    bool confidenceRight = FALSE;
    if (skel.leftHand_.confidence_ > 0.5 && skel.leftElbow_.confidence_ > 0.5 && skel.leftShoulder_.confidence_ > 0.5)
        confidenceLeft = TRUE;
    if (skel.rightHand_.confidence_ > 0.5 && skel.rightElbow_.confidence_ > 0.5 && skel.rightShoulder_.confidence_ > 0.5)
        confidenceRight = TRUE;
    
    // 3. draw active cursor if threshold reached
    if((skel.rightShoulder_.z_ - skel.rightHand_.z_) > depthThreshold && confidenceRight)
    {
        deadCursor_ = FALSE;
        float oldX, oldY;
        
        // if we are not active, calculate hover time and move cursor (4)
        if(!active_)
        {
            //update marker
            displayGroup_->getMarker()->setPosition(normX, normY);
            
            boost::shared_ptr<ContentWindowInterface> cwi = displayGroup_->getContentWindowInterfaceUnderMarker();
            
            // no window under cursor
            if (cwi == NULL)
            {
                printf("cwi is null\n");
                hoverTime_.restart();
                
                // set active window to NULL
                activeWindow_ = boost::shared_ptr<ContentWindowInterface>();
            }
            
            else
            {
                // first time cursor is over window
                if(activeWindow_ == NULL)
                {
                    activeWindow_ = cwi;
                }
                
                printf("hovertime = %d\n", hoverTime_.elapsed());
                
                // we have hovered long enough to make the window active and move it
                if(hoverTime_.elapsed() > HOVER_TIME)
                    active_ = TRUE;
            }
        }
        
        // else if we are active but not focused
        else if(!focusInteraction_ && active_)
        {
            //update marker
            displayGroup_->getMarker()->getPosition(oldX, oldY);
            displayGroup_->getMarker()->setPosition(normX, normY);
            
            // look for focus pose (6)
            int distanceHeadToLeftHand = calculateDistance(skel.leftHand_, skel.head_);
            int distanceElbowToShoulder = calculateDistance(skel.leftElbow_, skel.leftShoulder_);
            if (distanceHeadToLeftHand < distanceElbowToShoulder && confidenceLeft)
            {
                // set focused and start timeout for focused start
                focusInteraction_ = TRUE;
                focusTimeOut_.restart();
            }
            
            // left hand is active, scale window
            else if ((skel.leftShoulder_.z_ - skel.leftHand_.z_) > depthThreshold && confidenceLeft)
            {
                scaleWindow(skel.leftHand_, skel.rightHand_);
            }
            
            // left hand not present, move window under cursor
            else
            {
                scaling_ = FALSE;
                activeWindow_->setPosition(normX, normY);
            }
        }
        
        // active and focused
        else
        {
            scaling_ = FALSE;
            // if pose detected and timeout reached, exit active mode
            int distanceHeadToLeftHand = calculateDistance(skel.leftHand_, skel.head_);
            int distanceElbowToShoulder = calculateDistance(skel.leftElbow_, skel.leftShoulder_);
            if (distanceHeadToLeftHand < distanceElbowToShoulder && confidenceLeft)
            {
                if(focusTimeOut_.elapsed() > FOCUS_TIME)
                {
                    focusInteraction_ = active_ = FALSE;
                    hoverTime_.restart();
                }
            }
            
            // else update zoom/pan as necessary (7)
            // left hand is active, zoom content
            else if ((skel.leftShoulder_.z_ - skel.leftHand_.z_) > depthThreshold && confidenceLeft)
            {
                zoom(skel.leftHand_, skel.rightHand_);
            }
            
            // left hand not present, pan content
            else
            {
                pan(skel.rightHand_);
            }
        }
    }
    
    // sometimes the confidence is lost for a brief moment, so use timeout before making changes
    else
    {
        // no cursors and this is first time it disappeared
        if(!deadCursor_)
        {
            deadCursor_ = TRUE;
            deadCursorTimeOut_.restart();
        }
        else
        {
            if (deadCursorTimeOut_.elapsed() > DEAD_CURSOR_TIME)
            {
                hoverTime_.restart();
                active_ = focusInteraction_ = scaling_ = FALSE;
                
                // set active window to NULL
                activeWindow_ = boost::shared_ptr<ContentWindowInterface>();

            }
        }
    } 
}

void SkeletonState::pan(SkeletonPoint& rh)
{

}

void SkeletonState::zoom(SkeletonPoint& lh, SkeletonPoint& rh)
{

}

void SkeletonState::scaleWindow(SkeletonPoint& lh, SkeletonPoint& rh)
{
    // we've entered the scale pose previously
    if(scaling_)
    {
        float dd = neutralScaleDistance_ - calculateDistance(lh,rh);
        double x,y;
        activeWindow_->getSize(x, y);
        activeWindow_->setSize(x + dd/neutralScaleDistance_*WINDOW_SCALING_FACTOR, y + dd/neutralScaleDistance_*WINDOW_SCALING_FACTOR);
    }
    
    // we are in the scale pose for the first time
    else
    {
        scaling_ = TRUE;
        neutralScaleDistance_ = calculateDistance(lh, rh);
    }
}
