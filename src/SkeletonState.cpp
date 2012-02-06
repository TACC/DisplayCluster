#include "SkeletonState.h"
#include "DisplayGroupJoystick.h"
#include "ContentWindowInterface.h"
#include "main.h"
#include "log.h"
#include "vector.h"

// 2 second hovertime constant
const int HOVER_TIME              = 2000;
// timeout for focus gesture
const int FOCUS_TIME              = 2000;
// timeout for no hand detected
const int DEAD_CURSOR_TIME        = 500;
// scale factor for window size scaling
const float WINDOW_SCALING_FACTOR = 0.05;
// calse factor for panning content
const float WINDOW_PAN_FACTOR = 0.015;

inline float calculateDistance(SkeletonPoint& a, SkeletonPoint& b)
{
    float result = sqrt(pow(a.x_ - b.x_, 2) +
                        pow(a.y_ - b.y_, 2) +
                        pow(a.z_ - b.z_, 2));
    return result;
}

SkeletonState::SkeletonState(): leftHandActive_(FALSE),
                                rightHandActive_(FALSE),
                                focusInteraction_(FALSE),
                                skeletonRep_(),
                                hasControl_(FALSE),
                                active_(FALSE),
                                deadCursor_(FALSE),
                                hoverTime_(),
                                focusTimeOut_(0,0,2,0),
                                controllerTimeOut_(0,0,2,0),
                                deadCursorTimeOut_(),
                                activeWindow_()
{
    if(g_mpiRank == 0)
    {
        boost::shared_ptr<DisplayGroupJoystick> dgj(new DisplayGroupJoystick(g_displayGroupManager));
        displayGroup_ = dgj;
    }

}

int SkeletonState::update(SkeletonRepresentation& skel)
{
    // copy the skeleton joints to member
    skeletonRep_ = skel;

    // the process:
    // 1. get hand locations
    // 2. find cursor position by normalizing hand positions to maximum reach
    // 3. draw active cursor if depth threshold exceeded but no activity
    // 4. updated state with new hover time and speed if applicable
    // 5. if not active state and if hover time exceeds threshold, set active and activate window under cursor
    // 6. if active state, check for interacting focus if not already enabled, if pose detected, enable focusing, otherwise update window position and size
    // 7. if interacting focus, process pan or zoom on content, if pose detected, disable active and focus

    // depth threshold - factor length of elbow to shoulder
    float depthThreshold = 1.2 * calculateDistance(skel.rightShoulder_, skel.rightElbow_);

    // 1. get normalized hand locations

    // magnitude of arm length
    float armLength = calculateDistance(skel.rightHand_, skel.rightElbow_) +
                      calculateDistance(skel.rightElbow_, skel.rightShoulder_);

    // the maximum distance that can be reached by arm while crossing depth plane
    float maxReach = sqrt(armLength*armLength - depthThreshold*depthThreshold);

    // 2. find cursor position (normalized)
    float normX = ((skel.rightHand_.x_ - skel.rightShoulder_.x_) + maxReach)/(2*maxReach);
    float normY = 1 - ((skel.rightHand_.y_ - skel.rightShoulder_.y_) + maxReach)/(2*maxReach);
    
    // are we confident in relevant joint positions?
    bool confidenceLeft  = FALSE;
    bool confidenceRight = FALSE;
    if (skel.leftHand_.confidence_ > 0.5 && skel.leftElbow_.confidence_ > 0.5 && skel.leftShoulder_.confidence_ > 0.5)
        confidenceLeft = TRUE;
    if (skel.rightHand_.confidence_ > 0.5 && skel.rightElbow_.confidence_ > 0.5 && skel.rightShoulder_.confidence_ > 0.5)
        confidenceRight = TRUE;
        
    if ((skel.leftShoulder_.z_ - skel.leftHand_.z_) > depthThreshold && confidenceLeft)
        leftHandActive_ = TRUE;
    else leftHandActive_ = FALSE;
    if((skel.rightShoulder_.z_ - skel.rightHand_.z_) > depthThreshold && confidenceRight)
        rightHandActive_ = TRUE;
    else rightHandActive_ = FALSE;

    // we are not the controlling user, so we should check for the signal
    if(!hasControl_)
    {
        // look for signal - the raising of left hand one neck length above head
        if(skel.leftHand_.y_ > (skel.head_.y_ + calculateDistance(skel.head_, skel.neck_)) 
            && confidenceLeft)
        {
            hasControl_ = TRUE;
            controllerTimeOut_.restart();
            return 1;
        }
        
        return 0;
    }

    // 3. draw active cursor if threshold reached
    if(rightHandActive_)
    {
        deadCursor_ = FALSE;
        rightHandActive_ = TRUE;
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

                // we have hovered long enough to make the window active and move it
                if(hoverTime_.elapsed() > HOVER_TIME)
                {
                    active_ = TRUE;
                    activeWindow_->moveToFront();
                }
            }
        }

        // else if we are active but not focused
        else if(!focusInteraction_ && active_)
        {
 
            // look for focus pose (6) - here raising left hand one neck length above head
            if(skel.leftHand_.y_ > (skel.head_.y_ + calculateDistance(skel.head_, skel.neck_)) 
                && confidenceLeft
                && focusTimeOut_.elapsed() > FOCUS_TIME
                && controllerTimeOut_.elapsed() > FOCUS_TIME)
            {
                // set focused and start timeout for focused start
                focusInteraction_ = TRUE;
                focusTimeOut_.restart();
            }

            // left hand is active, scale window
            else if(leftHandActive_)
            {
                leftHandActive_ = TRUE;
                scaleWindow(skel.leftHand_, skel.rightHand_, 2.0 * calculateDistance(skel.rightHand_, skel.rightElbow_));
                
                //update marker
                displayGroup_->getMarker()->getPosition(oldX, oldY);
                displayGroup_->getMarker()->setPosition(normX, normY);
            }

            // left hand not present, move window under cursor
            else
            {
                //update marker
                displayGroup_->getMarker()->getPosition(oldX, oldY);
                displayGroup_->getMarker()->setPosition(normX, normY);

                // find old marker position, find new marker position, move window by the difference
                double dx,dy;
                dx = oldX - normX;
                dy = oldY - normY;
                double oldCenterX, oldCenterY;
                activeWindow_->getPosition(oldCenterX, oldCenterY);
                activeWindow_->setPosition(oldCenterX - dx, oldCenterY - dy);

            }
        }

        // active and focused
        else
        {

            //update marker
            displayGroup_->getMarker()->getPosition(oldX, oldY);
            displayGroup_->getMarker()->setPosition(normX, normY);

            // if pose is detected and the focus timeout is exceeded, exit interaction mode
            if(skel.leftHand_.y_ > (skel.head_.y_ + calculateDistance(skel.head_, skel.neck_))
                && confidenceLeft
                && focusTimeOut_.elapsed() > FOCUS_TIME)
            {
                focusInteraction_ = false;
                focusTimeOut_.restart();
            }

            else if(leftHandActive_)
            {
                zoom(skel.leftHand_, skel.rightHand_, 2.0 * calculateDistance(skel.rightHand_, skel.rightElbow_));
            }

            // pan window about current center
            else
            {   
                pan(skel.rightHand_, skel.rightShoulder_, maxReach);
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
                active_ = focusInteraction_ = FALSE;
                
                // set active window to NULL
                activeWindow_ = boost::shared_ptr<ContentWindowInterface>();

            }
        }
    }
}

void SkeletonState::zoom(SkeletonPoint& lh, SkeletonPoint& rh, float threshold)
{

    float handDistance = calculateDistance(lh, rh) / threshold;
    float zoomFactor = 1. + (handDistance - 1) / 50.;

    activeWindow_->setZoom(zoomFactor * activeWindow_->getZoom());
}

void SkeletonState::pan(SkeletonPoint& rh, SkeletonPoint& rs, float maxReach)
{
    //float normX = ((skel.rightHand_.x_ - skel.rightShoulder_.x_) + maxReach)/(2*maxReach);

    float normX = (rh.x_ - rs.x_)/maxReach;
    float normY = -1. * (rh.y_ - rs.y_)/maxReach;

    double oldCenterX, oldCenterY;
    activeWindow_->getCenter(oldCenterX, oldCenterY);
    activeWindow_->setCenter(oldCenterX + normX*WINDOW_PAN_FACTOR/activeWindow_->getZoom(), oldCenterY + normY*WINDOW_PAN_FACTOR/activeWindow_->getZoom());
}

void SkeletonState::scaleWindow(SkeletonPoint& lh, SkeletonPoint& rh, float threshold)
{

    //float differenceFromThreshold = calculateDistance(lh, rh) - threshold;
    float scaleFactor = calculateDistance(lh, rh)/threshold;

    // scale window by 10% of scale factor
    if(scaleFactor > 1.0)
    {
        scaleFactor = 1 + (scaleFactor - 1)*WINDOW_SCALING_FACTOR;
    }
    else
        scaleFactor = 1 - (1 - scaleFactor)*WINDOW_SCALING_FACTOR;

    activeWindow_->scaleSize(scaleFactor);
}

void SkeletonState::render()
{

    glPushMatrix();

    glScalef(1./20000., 1./20000., 1./20000.);

    drawJoints();
    
    // color the limbs uniformly
    glColor4f(220./255.,220./255.,220./255.,1.);

    drawLimb(skeletonRep_.head_, skeletonRep_.neck_);
    drawLimb(skeletonRep_.neck_, skeletonRep_.leftShoulder_);
    drawLimb(skeletonRep_.leftShoulder_, skeletonRep_.leftElbow_);
    drawLimb(skeletonRep_.leftElbow_, skeletonRep_.leftHand_);
    drawLimb(skeletonRep_.neck_, skeletonRep_.rightShoulder_);
    drawLimb(skeletonRep_.rightShoulder_, skeletonRep_.rightElbow_);
    drawLimb(skeletonRep_.rightElbow_, skeletonRep_.rightHand_);
    //drawLimb(skeletonRep_.leftShoulder_, skeletonRep_.torso_);
    //drawLimb(skeletonRep_.rightShoulder_, skeletonRep_.torso_);
    drawLimb(skeletonRep_.neck_, skeletonRep_.torso_);
    drawLimb(skeletonRep_.torso_, skeletonRep_.leftHip_);
    drawLimb(skeletonRep_.leftHip_, skeletonRep_.leftKnee_);
    drawLimb(skeletonRep_.leftKnee_, skeletonRep_.leftFoot_);
    drawLimb(skeletonRep_.torso_, skeletonRep_.rightHip_);
    drawLimb(skeletonRep_.rightHip_, skeletonRep_.rightKnee_);
    drawLimb(skeletonRep_.rightKnee_, skeletonRep_.rightFoot_);
    drawLimb(skeletonRep_.leftHip_, skeletonRep_.rightHip_);

    glPopMatrix();
}

void SkeletonState::drawJoints()
{
    std::vector<SkeletonPoint> joints;
    joints.push_back(skeletonRep_.head_);
    joints.push_back(skeletonRep_.neck_);
    joints.push_back(skeletonRep_.leftShoulder_);
    joints.push_back(skeletonRep_.leftElbow_);
    joints.push_back(skeletonRep_.leftHand_);
    joints.push_back(skeletonRep_.rightShoulder_);
    joints.push_back(skeletonRep_.rightElbow_);
    joints.push_back(skeletonRep_.rightHand_);
    joints.push_back(skeletonRep_.torso_);
    joints.push_back(skeletonRep_.leftHip_);
    joints.push_back(skeletonRep_.rightHip_);
    joints.push_back(skeletonRep_.leftKnee_);
    joints.push_back(skeletonRep_.rightKnee_);
    joints.push_back(skeletonRep_.leftFoot_);
    joints.push_back(skeletonRep_.rightFoot_);
    
    // set up glu object
    GLUquadricObj* quadobj;
    quadobj = gluNewQuadric();

    for(unsigned int i = 0; i < joints.size(); i++)
    {
        // default color for joints
        glColor4f(168./255.,187./255.,219./255.,1.);
        
        if(hasControl_)
            glColor4f(70./255., 219./255., 147./255., 1.);
        
        if (joints[i].confidence_ > 0.5)
        {
        
            // if it's the head
            if(i == 0)
            {
                // color it if in interaction mode
                if (focusInteraction_)
                    glColor4f(183./255., 105./255., 255./255., 1.);
                // make it larger
                glPushMatrix();
                glTranslatef(joints[i].x_, joints[i].y_, joints[i].z_);
                gluSphere(quadobj,60.,16.,16.);
                glPopMatrix();
            }
        
            // if it's the left hand and active
            if(i == 4 && leftHandActive_)
            {
                // color the hand red and make it larger
                glColor4f(1.0, 0.0, 0.0, 1);
                glPushMatrix();
                glTranslatef(joints[i].x_, joints[i].y_, joints[i].z_);
                gluSphere(quadobj,50.,16.,16.);
                glPopMatrix();
            }

            // if it's the right and hand active
            else if(i == 7 && rightHandActive_)
            {
                // color the hand red and make it larger
                glColor4f(1.0, 0.0, 0.0, 1);
                glPushMatrix();
                glTranslatef(joints[i].x_, joints[i].y_, joints[i].z_);
                gluSphere(quadobj,50.,16.,16.);
                glPopMatrix();
            }

            else
            {
                glPushMatrix();
                glTranslatef(joints[i].x_, joints[i].y_, joints[i].z_);
                gluSphere(quadobj,30.,16.,16.);
                glPopMatrix();
            }
        }
    }
}

void SkeletonState::drawLimb(SkeletonPoint& pt1, SkeletonPoint& pt2)
{
    
    if(pt1.confidence_ <= 0.5 || pt2.confidence_ <= 0.5)
        return;

    float a[3] = {pt1.x_, pt1.y_, pt1.z_};
    float b[3] = {pt2.x_, pt2.y_, pt2.z_};

    // vector formed by the two joints
    float c[3];
    vectorSubtraction(a, b, c);

    // glu cylinder vector
    float z[3] = {0,0,1};

    // r is axis of rotation about z
    float r[3];
    vectorCrossProduct(z, c, r);

    // get angle of rotation in degrees
    float angle = 180/M_PI * acos((vectorDotProduct(z, c)/vectorMagnitude(c)));

    glPushMatrix();

    // translate to second joint
    glTranslatef(pt2.x_, pt2.y_, pt2.z_);
    glRotatef(angle, r[0], r[1], r[2]);

    // set up glu object
    GLUquadricObj* quadobj;
    quadobj = gluNewQuadric();

    gluCylinder(quadobj, 10, 10, vectorMagnitude(c), 10, 10);

    glPopMatrix();

    // delete used quadric
    gluDeleteQuadric(quadobj);
}
