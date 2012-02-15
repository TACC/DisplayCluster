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
    if (rc != XN_STATUS_OK)
    {
        put_flog(LOG_ERROR, "%s failed: %s\n", description, xnGetStatusString(rc));
        return -1;
    }
}

SkeletonSensor::SkeletonSensor() :  context_(),
                                    depthG_(),
                                    userG_(),
                                    pointModeProjective_(FALSE),
                                    pose_("Psi"),
                                    trackedUsers_(),
                                    smoothingFactor_(0.9)
{}

SkeletonSensor::~SkeletonSensor()
{
    context_.Shutdown();
}

int SkeletonSensor::initialize()
{
    context_.Init();

    XnStatus rc = XN_STATUS_OK;

    XnMapOutputMode mapMode;

    // create depth and user generators
    rc = depthG_.Create(context_);
    if (CHECK_RC(rc, "Create depth generator") == -1)
        return -1;
    rc = userG_.Create(context_);
    if (CHECK_RC(rc, "Create user generator") == -1)
        return -1;

    depthG_.GetMapOutputMode(mapMode);

    // for now, make output map VGA resolution at 30 FPS
    mapMode.nXRes = XN_VGA_X_RES;
    mapMode.nYRes = XN_VGA_Y_RES;
    mapMode.nFPS  = 30;

    depthG_.SetMapOutputMode(mapMode);

    // turn on device mirroring
    if(TRUE == depthG_.IsCapabilitySupported("Mirror"))
    {
        rc = depthG_.GetMirrorCap().SetMirror(TRUE);
        CHECK_RC(rc, "Setting Image Mirroring on depthG");
    }

    // make sure the user points are reported from the POV of the depth generator
    userG_.GetAlternativeViewPointCap().SetViewPoint(depthG_);
    userG_.GetSkeletonCap().SetSmoothing(smoothingFactor_);

    // start data streams
    context_.StartGeneratingAll();

    setCalibrationPoseCallbacks();

    return 0;
}

bool SkeletonSensor::updateTrackedUsers()
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

std::vector<Skeleton> SkeletonSensor::getSkeletons()
{
    std::vector<Skeleton> skeletons;
    
    for(unsigned int i = 0; i < getNOTrackedUsers(); i++)
    {
        Skeleton s = getSkeleton(trackedUsers_[i]);
        skeletons.push_back(s);
    }

    return skeletons;
}

// converts the OpenNI positions to simple 3D points
void SkeletonSensor::convertXnJointToPoint(XnSkeletonJointPosition* const joints, SkeletonPoint* const points, unsigned int numPoints)
{

    XnPoint3D xpt;
    for(unsigned int i = 0; i < numPoints; i++)
    {
        xpt = joints[i].position;
        if(pointModeProjective_)
            depthG_.ConvertRealWorldToProjective(1, &xpt, &xpt);

        points[i].confidence_ = joints[i].fConfidence;
        points[i].x_ = xpt.X;
        points[i].y_ = xpt.Y;
        points[i].z_ = xpt.Z;
    }
}

Skeleton SkeletonSensor::getSkeleton(const unsigned int UID)
{
    Skeleton result;
    // not tracking user
    if(!userG_.GetSkeletonCap().IsTracking(UID))
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
    XnSkeletonJointPosition positions[15];

    for (unsigned int i = 0; i < nJoints; i++)
    {
        userG_.GetSkeletonCap().GetSkeletonJointPosition(UID, joints[i], *(positions+i));
    }

    SkeletonPoint points[15];
    convertXnJointToPoint(positions, points, 15);

    result.head_              = points[0];
    result.neck_              = points[1];
    result.rightShoulder_     = points[2];
    result.leftShoulder_      = points[3];
    result.rightElbow_        = points[4];
    result.leftElbow_         = points[5];
    result.rightHand_         = points[6];
    result.leftHand_          = points[7];
    result.rightHip_          = points[8];
    result.leftHip_           = points[9];
    result.rightKnee_         = points[10];
    result.leftKnee_          = points[11];
    result.rightFoot_         = points[12];
    result.leftFoot_          = points[13];
    result.torso_             = points[14];

    return result;
}

int SkeletonSensor::getClosestTrackedUID()
{
    XnPoint3D CoMTemp;
    XnPoint3D closestCoM;
    closestCoM.Z = -1.0; // a known default value
    int nearestUID = -1; // a known default value
    for(unsigned int i = 0; i < trackedUsers_.size(); i++)
    {
        // get user's CoM z component
        // compare to any previous z, if lower save user for later
        userG_.GetCoM(trackedUsers_[i], CoMTemp);
        if(closestCoM.Z != -1.0)
        {
            // this user is closer to sensor than any previous tracked user
            if(CoMTemp.Z < closestCoM.Z)
            {
                closestCoM.Z = CoMTemp.Z;
                nearestUID = trackedUsers_[i];
            }
        }

        // first time through loop, first user is closest
        else
        {
            closestCoM.Z = CoMTemp.Z;
            nearestUID = trackedUsers_[i];
        }
    }
    return nearestUID;
}

// Kinect: 1 Pose: "Psi"
void SkeletonSensor::printAvailablePoses()
{
    XnUInt32 numPoses = userG_.GetPoseDetectionCap().GetNumberOfPoses();

    put_flog(LOG_DEBUG, "Number of poses: %d.\n", numPoses);
}

// set device to look for calibration pose and supply callback functions for user events
int SkeletonSensor::setCalibrationPoseCallbacks()
{
    XnCallbackHandle hUserCallbacks, hCalibrationStart, hCalibrationComplete, hPoseDetected;
    XnStatus rc = XN_STATUS_OK;

    userG_.RegisterUserCallbacks(newUserCallback, lostUserCallback, this, hUserCallbacks);
    userG_.GetSkeletonCap().RegisterToCalibrationStart(calibrationStartCallback, this, hCalibrationStart);
    userG_.GetSkeletonCap().RegisterToCalibrationComplete(calibrationCompleteCallback, this, hCalibrationComplete);

    // turn on tracking of all joints
    userG_.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

    return 0;
}

void XN_CALLBACK_TYPE SkeletonSensor::newUserCallback(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
    SkeletonSensor* sensor = (SkeletonSensor*) pCookie;
    put_flog(LOG_DEBUG, "New User %d\n", nId);

    // auto-calibrate user
    put_flog(LOG_DEBUG, "Auto-calibrating user %d.\n", nId);
    sensor->getUserGenerator()->GetSkeletonCap().RequestCalibration(nId,TRUE);
}

// Callback: An existing user was lost
void XN_CALLBACK_TYPE SkeletonSensor::lostUserCallback(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
    put_flog(LOG_DEBUG, "Lost user %d\n", nId);
}

void XN_CALLBACK_TYPE SkeletonSensor::calibrationStartCallback(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie)
{
    put_flog(LOG_DEBUG, "Calibration started for user %d\n", nId);
}

void XN_CALLBACK_TYPE SkeletonSensor::calibrationCompleteCallback(xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus eStatus, void* pCookie)
{
    SkeletonSensor* sensor = (SkeletonSensor*) pCookie;
    if (eStatus == XN_CALIBRATION_STATUS_OK)
    {
        // Calibration succeeded
        printf("Calibration completed: Start tracking user %d\n", nId);
        sensor->getUserGenerator()->GetSkeletonCap().StartTracking(nId);
    }
    else
    {
        // Calibration failed
        put_flog(LOG_DEBUG, "Calibration failed for user %d\n", nId);

        sensor->getUserGenerator()->GetSkeletonCap().RequestCalibration(nId, TRUE);
    }
}

void XN_CALLBACK_TYPE SkeletonSensor::poseDetectedCallback(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie)
{
    SkeletonSensor* sensor = (SkeletonSensor*) pCookie;
    put_flog(LOG_DEBUG, "Pose %s detected for user %d\n", sensor->getPoseString(), nId);
    sensor->getUserGenerator()->GetPoseDetectionCap().StopPoseDetection(nId);
    sensor->getUserGenerator()->GetSkeletonCap().RequestCalibration(nId, TRUE);
}
