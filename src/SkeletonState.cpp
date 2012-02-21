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

#include "SkeletonState.h"
#include "DisplayGroupJoystick.h"
#include "ContentWindowInterface.h"
#include "main.h"
#include "log.h"
#include "vector.h"

#ifdef __APPLE__
    #include <OpenGL/glu.h>
#else
    #include <GL/glu.h>
#endif

inline float calculateDistance(SkeletonPoint& a, SkeletonPoint& b)
{
    float result = sqrtf(pow(a.x - b.x, 2) +
                        pow(a.y - b.y, 2) +
                        pow(a.z - b.z, 2));
    return result;
}

SkeletonState::SkeletonState()
{
    if(g_mpiRank == 0)
    {
        // create display group interface
        boost::shared_ptr<DisplayGroupJoystick> dgj(new DisplayGroupJoystick(g_displayGroupManager));
        displayGroup_ = dgj;

        // default mode is focused interaction
        focusInteraction_ = true;

        // not in control
        control_ = false;

        // hands inactive
        leftHandActive_ = rightHandActive_ = false;

        // don't have an active window
        activeWindow_ = false;

        // start timers
        modeChangeTimeOut_.start();
    }
}

int SkeletonState::update(Skeleton skeleton)
{
    // copy the skeleton joints to member
    skeleton_ = skeleton;

    // are we confident in joint positions?
    bool confidentLeft  = false;
    bool confidentRight = false;

    if(skeleton_.leftHand.confidence > 0.5 && skeleton_.leftElbow.confidence > 0.5 && skeleton_.leftShoulder.confidence > 0.5)
        confidentLeft = true;

    if(skeleton_.rightHand.confidence > 0.5 && skeleton_.rightElbow.confidence > 0.5 && skeleton_.rightShoulder.confidence > 0.5)
        confidentRight = true;

    // if we are not a controlling user, check for the signal and return
    if(control_ == false)
    {
        // make sure hands are marked inactive
        leftHandActive_ = rightHandActive_ = false;

        // the raising of left hand above head
        if(confidentLeft && skeleton_.leftHand.y > skeleton_.head.y)
        {
            control_ = true;

            controlTimeOut_.restart();

            return 1;
        }
        else
            return 0;
    }

    // depth threshold - factor length of elbow to shoulder
    float depthThreshold = 1.2 * calculateDistance(skeleton_.rightShoulder, skeleton_.rightElbow);

    // magnitude of arm length
    float armLength = calculateDistance(skeleton_.rightHand, skeleton_.rightElbow) +
                      calculateDistance(skeleton_.rightElbow, skeleton_.rightShoulder);

    // the maximum distance that can be reached by arm while crossing depth plane
    float maxReach = sqrtf(armLength*armLength - depthThreshold*depthThreshold);

    // marker position (normalized)
    float normX = ((skeleton_.rightHand.x - skeleton_.rightShoulder.x) + maxReach)/(2.*maxReach);
    float normY = 1. - ((skeleton_.rightHand.y - skeleton_.rightShoulder.y) + maxReach)/(2.*maxReach);

    // determine if hands are active
    if(confidentLeft && (skeleton_.leftShoulder.z - skeleton_.leftHand.z) > depthThreshold)
        leftHandActive_ = true;
    else
        leftHandActive_ = false;

    if(confidentRight && (skeleton_.rightShoulder.z - skeleton_.rightHand.z) > depthThreshold)
        rightHandActive_ = true;
    else
        rightHandActive_ = false;

    // update marker if right hand is active
    if(rightHandActive_)
    {
        // restart marker timer
        markerTimeOut_.restart();

        // keep track of old position
        float oldX, oldY;
        displayGroup_->getMarker()->getPosition(oldX, oldY);

        // set new position
        displayGroup_->getMarker()->setPosition(normX, normY);

        // if we do not have an active window
        if(activeWindow_ == false)
        {
            // see if we have a window under the marker
            boost::shared_ptr<ContentWindowInterface> cwi = displayGroup_->getContentWindowInterfaceUnderMarker();

            // no window under marker
            if(cwi != NULL)
            {
                // first time marker is over window, or the window under marker has changed
                if(hoverWindow_ == NULL || hoverWindow_ != cwi)
                {
                    hoverTime_.restart();
                    hoverWindow_ = cwi;
                }

                // we have hovered long enough to make the window active and move it to front
                if(hoverTime_.elapsed() > HOVER_TIME)
                {
                    activeWindow_ = true;
                    hoverWindow_->moveToFront();

                    // highlight the window
                    hoverWindow_->highlight();
                }
            }
        }
        else
        {
            // we have an active window

            // look for mode change pose: hand over head
            if(confidentLeft && skeleton_.leftHand.y > skeleton_.head.y && modeChangeTimeOut_.elapsed() > MODE_CHANGE_TIME && controlTimeOut_.elapsed() > MODE_CHANGE_TIME)
            {
                // change mode and reset timer
                focusInteraction_ = !focusInteraction_;
                modeChangeTimeOut_.restart();
            }

            // neutral distance for zooming, scaling
            float neutralDistance = 2. * calculateDistance(skeleton_.rightHand, skeleton_.rightElbow);

            if(focusInteraction_ == true)
            {
                // left hand is active, zoom in window
                if(leftHandActive_)
                {
                    float zoomFactor = calculateDistance(skeleton_.leftHand, skeleton_.rightHand) / neutralDistance;

                    zoomFactor = 1. + (zoomFactor - 1.)*WINDOW_ZOOM_FACTOR;

                    hoverWindow_->setZoom(zoomFactor * hoverWindow_->getZoom());
                }
                else
                {
                    // left hand not present (but we do have right hand), pan in window
                    double oldCenterX, oldCenterY;
                    hoverWindow_->getCenter(oldCenterX, oldCenterY);

                    hoverWindow_->setCenter(oldCenterX + (normX-0.5)*WINDOW_PAN_FACTOR/hoverWindow_->getZoom(), oldCenterY + (normY-0.5)*WINDOW_PAN_FACTOR/hoverWindow_->getZoom());
                }
            }
            else
            {
                // left hand is active, scale window
                if(leftHandActive_)
                {
                    float scaleFactor = calculateDistance(skeleton_.leftHand, skeleton_.rightHand) / neutralDistance;

                    scaleFactor = 1. + (scaleFactor - 1.)*WINDOW_SCALE_FACTOR;

                    hoverWindow_->scaleSize(scaleFactor);
                }
                else
                {
                    // left hand not present (but we do have right hand), move window under marker
                    double dx = normX - oldX;
                    double dy = normY - oldY;

                    double oldPositionX, oldPositionY;
                    hoverWindow_->getPosition(oldPositionX, oldPositionY);

                    hoverWindow_->setPosition(oldPositionX + dx, oldPositionY + dy);
                }
            }
        }
    }
    else
    {
        // right hand not active

        // sometimes the confidence is lost for a brief moment, so use timeout before making changes
        if(markerTimeOut_.elapsed() > DEAD_MARKER_TIME)
        {
            // no longer have an active window; set hover window to NULL
            activeWindow_ = false;
            hoverWindow_ = boost::shared_ptr<ContentWindowInterface>();

            // the first mode should always be focus interaction
            focusInteraction_ = true;
        }
    }

    return 0;
}

void SkeletonState::render()
{
    glPushAttrib(GL_CURRENT_BIT);
    glPushMatrix();

    // scale appropriately to fit in view
    glScalef(1./10000., 1./10000., 1./10000.);

    renderJoints();

    // color of the limbs
    glColor4f(220./255.,220./255.,220./255.,1.);

    renderLimb(skeleton_.head, skeleton_.neck);
    renderLimb(skeleton_.neck, skeleton_.leftShoulder);
    renderLimb(skeleton_.leftShoulder, skeleton_.leftElbow);
    renderLimb(skeleton_.leftElbow, skeleton_.leftHand);
    renderLimb(skeleton_.neck, skeleton_.rightShoulder);
    renderLimb(skeleton_.rightShoulder, skeleton_.rightElbow);
    renderLimb(skeleton_.rightElbow, skeleton_.rightHand);
    renderLimb(skeleton_.neck, skeleton_.torso);
    renderLimb(skeleton_.torso, skeleton_.leftHip);
    renderLimb(skeleton_.leftHip, skeleton_.leftKnee);
    renderLimb(skeleton_.leftKnee, skeleton_.leftFoot);
    renderLimb(skeleton_.torso, skeleton_.rightHip);
    renderLimb(skeleton_.rightHip, skeleton_.rightKnee);
    renderLimb(skeleton_.rightKnee, skeleton_.rightFoot);
    renderLimb(skeleton_.leftHip, skeleton_.rightHip);

    glPopMatrix();
    glPopAttrib();
}

bool SkeletonState::getControl()
{
    return control_;
}

void SkeletonState::setControl(bool control)
{
    control_ = control;
}

void SkeletonState::renderJoints()
{
    std::vector<SkeletonPoint> joints;

    joints.push_back(skeleton_.head);
    joints.push_back(skeleton_.neck);
    joints.push_back(skeleton_.leftShoulder);
    joints.push_back(skeleton_.leftElbow);
    joints.push_back(skeleton_.leftHand);
    joints.push_back(skeleton_.rightShoulder);
    joints.push_back(skeleton_.rightElbow);
    joints.push_back(skeleton_.rightHand);
    joints.push_back(skeleton_.torso);
    joints.push_back(skeleton_.leftHip);
    joints.push_back(skeleton_.rightHip);
    joints.push_back(skeleton_.leftKnee);
    joints.push_back(skeleton_.rightKnee);
    joints.push_back(skeleton_.leftFoot);
    joints.push_back(skeleton_.rightFoot);

    // set up quadric object
    GLUquadricObj * quadObj = gluNewQuadric();

    for(unsigned int i = 0; i < joints.size(); i++)
    {
        glPushMatrix();
        glTranslatef(joints[i].x, joints[i].y, joints[i].z);

        // default color for joints
        glColor4f(168./255.,187./255.,219./255.,1.);

        // default sphere radius
        float radius = 30.;

        // if we're in control and in focus interaction mode, color green
        // if we're in control and not in focus interaction mode, color blue
        if(control_ && focusInteraction_ == true)
            glColor4f(70./255., 219./255., 147./255., 1.);
        else if(control_ && focusInteraction_ == false)
            glColor4f(0.,0.,1.,1.);

        if(joints[i].confidence > 0.5)
        {
            // if it's the head
            if(i == 0)
            {
                gluSphere(quadObj, 2.*radius, 16.,16.);
            }

            // if it's the left or right hand and active
            else if((i == 4 && leftHandActive_) || (i == 7 && rightHandActive_))
            {
                // color the hand red and make it larger
                glColor4f(1.0, 0.0, 0.0, 1);
                gluSphere(quadObj, 1.7*radius, 16.,16.);
            }
            else
            {
                gluSphere(quadObj, radius, 16.,16.);
            }
        }

        glPopMatrix();
    }

    // delete used quadric
    gluDeleteQuadric(quadObj);
}

void SkeletonState::renderLimb(SkeletonPoint& pt1, SkeletonPoint& pt2)
{
    if(pt1.confidence <= 0.5 || pt2.confidence <= 0.5)
        return;

    float a[3] = {pt1.x, pt1.y, pt1.z};
    float b[3] = {pt2.x, pt2.y, pt2.z};

    // vector formed by the two joints
    float c[3];
    vectorSubtraction(a, b, c);

    // glu cylinder vector
    float z[3] = {0,0,1};

    // r is axis of rotation about z
    float r[3];
    vectorCrossProduct(z, c, r);

    // get angle of rotation in degrees
    float angle = 180./M_PI * acos((vectorDotProduct(z, c)/vectorMagnitude(c)));

    glPushMatrix();

    // translate to second joint
    glTranslatef(pt2.x, pt2.y, pt2.z);
    glRotatef(angle, r[0], r[1], r[2]);

    // set up quadric object
    GLUquadricObj* quadObj = gluNewQuadric();

    gluCylinder(quadObj, 10, 10, vectorMagnitude(c), 10, 10);

    glPopMatrix();

    // delete used quadric
    gluDeleteQuadric(quadObj);
}
