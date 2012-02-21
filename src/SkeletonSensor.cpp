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

#include "SkeletonSensor.h"
#include "log.h"

inline int CHECK_RC(const unsigned int rc, const char* const description)
{
    if(rc != XN_STATUS_OK)
    {
        put_flog(LOG_ERROR, "%s failed: %s", description, xnGetStatusString(rc));
        return -1;
    }

    return 0;
}

SkeletonSensor::SkeletonSensor()
{
    pointModeProjective_ = false;
}

SkeletonSensor::~SkeletonSensor()
{
    context_.Shutdown();
}

int SkeletonSensor::initialize()
{
    context_.Init();

    XnStatus rc = XN_STATUS_OK;

    // create depth and user generators
    rc = depthG_.Create(context_);

    if(CHECK_RC(rc, "Create depth generator") == -1)
        return -1;

    rc = userG_.Create(context_);

    if(CHECK_RC(rc, "Create user generator") == -1)
        return -1;

    XnMapOutputMode mapMode;
    depthG_.GetMapOutputMode(mapMode);

    // for now, make output map VGA resolution at 30 FPS
    mapMode.nXRes = XN_VGA_X_RES;
    mapMode.nYRes = XN_VGA_Y_RES;
    mapMode.nFPS  = 30;

    depthG_.SetMapOutputMode(mapMode);

    // turn on device mirroring
    if(depthG_.IsCapabilitySupported("Mirror") == true)
    {
        rc = depthG_.GetMirrorCap().SetMirror(true);
        CHECK_RC(rc, "Setting Image Mirroring on depthG");
    }

    // make sure the user points are reported from the POV of the depth generator
    userG_.GetAlternativeViewPointCap().SetViewPoint(depthG_);

    // set smoothing factor
    userG_.GetSkeletonCap().SetSmoothing(0.9);

    // start data streams
    context_.StartGeneratingAll();

    // setup callbacks
    setCalibrationPoseCallbacks();

    return 0;
}

void SkeletonSensor::waitForDeviceUpdateOnUser()
{
    context_.WaitOneUpdateAll(userG_);
}

void SkeletonSensor::updateTrackedUsers()
{
    XnUserID users[64];
    XnUInt16 nUsers = userG_.GetNumberOfUsers();

    trackedUsers_.clear();

    userG_.GetUsers(users, nUsers);

    for(int i = 0; i < nUsers; i++)
    {
        if(userG_.GetSkeletonCap().IsTracking(users[i]))
        {
            trackedUsers_.push_back(users[i]);
        }
    }
}

bool SkeletonSensor::isTracking(const unsigned int uid)
{
    return userG_.GetSkeletonCap().IsTracking(uid);
}

Skeleton SkeletonSensor::getSkeleton(const unsigned int uid)
{
    Skeleton result;

    // not tracking user
    if(!userG_.GetSkeletonCap().IsTracking(uid))
        return result;

    // Array of available joints
    const unsigned int nJoints = 15;
    XnSkeletonJoint joints[nJoints] = 
    {   XN_SKEL_HEAD,
        XN_SKEL_NECK,
        XN_SKEL_RIGHT_SHOULDER,
        XN_SKEL_LEFT_SHOULDER,
        XN_SKEL_RIGHT_ELBOW,
        XN_SKEL_LEFT_ELBOW,
        XN_SKEL_RIGHT_HAND,
        XN_SKEL_LEFT_HAND,
        XN_SKEL_RIGHT_HIP,
        XN_SKEL_LEFT_HIP,
        XN_SKEL_RIGHT_KNEE,
        XN_SKEL_LEFT_KNEE,
        XN_SKEL_RIGHT_FOOT,
        XN_SKEL_LEFT_FOOT,
        XN_SKEL_TORSO 
    };

    // holds the joint position components
    XnSkeletonJointPosition positions[nJoints];

    for (unsigned int i = 0; i < nJoints; i++)
    {
        userG_.GetSkeletonCap().GetSkeletonJointPosition(uid, joints[i], *(positions+i));
    }

    SkeletonPoint points[15];
    convertXnJointsToPoints(positions, points, nJoints);

    result.head              = points[0];
    result.neck              = points[1];
    result.rightShoulder     = points[2];
    result.leftShoulder      = points[3];
    result.rightElbow        = points[4];
    result.leftElbow         = points[5];
    result.rightHand         = points[6];
    result.leftHand          = points[7];
    result.rightHip          = points[8];
    result.leftHip           = points[9];
    result.rightKnee         = points[10];
    result.leftKnee          = points[11];
    result.rightFoot         = points[12];
    result.leftFoot          = points[13];
    result.torso             = points[14];

    return result;
}

std::vector<Skeleton> SkeletonSensor::getSkeletons()
{
    std::vector<Skeleton> skeletons;

    for(unsigned int i = 0; i < getNumTrackedUsers(); i++)
    {
        Skeleton s = getSkeleton(trackedUsers_[i]);
        skeletons.push_back(s);
    }

    return skeletons;
}

unsigned int SkeletonSensor::getNumTrackedUsers()
{
    return trackedUsers_.size();
}

unsigned int SkeletonSensor::getUID(const unsigned int index)
{
    return trackedUsers_[index];
}

void SkeletonSensor::setPointModeToProjective()
{
    pointModeProjective_ = true;
}

void SkeletonSensor::setPointModeToReal()
{
    pointModeProjective_ = false;
}

int SkeletonSensor::setCalibrationPoseCallbacks()
{
    XnCallbackHandle hUserCallbacks, hCalibrationStart, hCalibrationComplete;

    userG_.RegisterUserCallbacks(newUserCallback, lostUserCallback, this, hUserCallbacks);
    userG_.GetSkeletonCap().RegisterToCalibrationStart(calibrationStartCallback, this, hCalibrationStart);
    userG_.GetSkeletonCap().RegisterToCalibrationComplete(calibrationCompleteCallback, this, hCalibrationComplete);

    // turn on tracking of all joints
    userG_.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

    return 0;
}

void SkeletonSensor::convertXnJointsToPoints(XnSkeletonJointPosition* const joints, SkeletonPoint* const points, unsigned int numPoints)
{
    XnPoint3D xpt;

    for(unsigned int i = 0; i < numPoints; i++)
    {
        xpt = joints[i].position;

        if(pointModeProjective_)
            depthG_.ConvertRealWorldToProjective(1, &xpt, &xpt);

        points[i].confidence = joints[i].fConfidence;
        points[i].x = xpt.X;
        points[i].y = xpt.Y;
        points[i].z = xpt.Z;
    }
}

void XN_CALLBACK_TYPE SkeletonSensor::newUserCallback(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
    put_flog(LOG_DEBUG, "New user %d, auto-calibrating", nId);

    SkeletonSensor* sensor = (SkeletonSensor*) pCookie;
    sensor->userG_.GetSkeletonCap().RequestCalibration(nId, true);
}

void XN_CALLBACK_TYPE SkeletonSensor::lostUserCallback(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
    put_flog(LOG_DEBUG, "Lost user %d", nId);
}

void XN_CALLBACK_TYPE SkeletonSensor::calibrationStartCallback(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie)
{
    put_flog(LOG_DEBUG, "Calibration started for user %d", nId);
}

void XN_CALLBACK_TYPE SkeletonSensor::calibrationCompleteCallback(xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus eStatus, void* pCookie)
{
    SkeletonSensor* sensor = (SkeletonSensor*) pCookie;

    if(eStatus == XN_CALIBRATION_STATUS_OK)
    {
        put_flog(LOG_DEBUG, "Calibration completed: start tracking user %d", nId);

        sensor->userG_.GetSkeletonCap().StartTracking(nId);
    }
    else
    {
        put_flog(LOG_DEBUG, "Calibration failed for user %d", nId);

        sensor->userG_.GetSkeletonCap().RequestCalibration(nId, true);
    }
}

void XN_CALLBACK_TYPE SkeletonSensor::poseDetectedCallback(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie)
{
    put_flog(LOG_DEBUG, "Pose detected for user %d", nId);

    SkeletonSensor* sensor = (SkeletonSensor*) pCookie;

    sensor->userG_.GetPoseDetectionCap().StopPoseDetection(nId);
    sensor->userG_.GetSkeletonCap().RequestCalibration(nId, true);
}
