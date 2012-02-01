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
                                    smoothingFactor_(0.8),
                                    needCalibrationPose_(FALSE)
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
    
    return 1;
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
        
void SkeletonSensor::getHandPoints(const unsigned int i, SkeletonPoint* const hands)
{
    XnSkeletonJointPosition joints[2];
    userG_.GetSkeletonCap().GetSkeletonJointPosition(trackedUsers_[i], XN_SKEL_LEFT_HAND, joints[0]);
    userG_.GetSkeletonCap().GetSkeletonJointPosition(trackedUsers_[i], XN_SKEL_RIGHT_HAND, joints[1]);
    
    convertXnJointToPoint(joints, hands, 2);
}

void SkeletonSensor::getElbowPoints(const unsigned int i, SkeletonPoint* const elbows)
{
    XnSkeletonJointPosition joints[2];
    userG_.GetSkeletonCap().GetSkeletonJointPosition(trackedUsers_[i], XN_SKEL_LEFT_ELBOW, joints[0]);
    userG_.GetSkeletonCap().GetSkeletonJointPosition(trackedUsers_[i], XN_SKEL_RIGHT_ELBOW, joints[1]);
    convertXnJointToPoint(joints, elbows, 2);

}

void SkeletonSensor::getArmPoints(const unsigned int i, SkeletonPoint* const arms)
{
    getHandPoints(i, arms);
    getElbowPoints(i, arms+2);
    
    XnSkeletonJointPosition joints[2];
    userG_.GetSkeletonCap().GetSkeletonJointPosition(trackedUsers_[i], XN_SKEL_LEFT_ELBOW, joints[4]);
    userG_.GetSkeletonCap().GetSkeletonJointPosition(trackedUsers_[i], XN_SKEL_RIGHT_ELBOW, joints[5]);
    convertXnJointToPoint(joints, arms, 2);
}

void SkeletonSensor::getShoulderPoints(const unsigned int i, SkeletonPoint* const shoulders)
{
    XnSkeletonJointPosition joints[2];
    userG_.GetSkeletonCap().GetSkeletonJointPosition(trackedUsers_[i], XN_SKEL_LEFT_SHOULDER, joints[0]);
    userG_.GetSkeletonCap().GetSkeletonJointPosition(trackedUsers_[i], XN_SKEL_RIGHT_SHOULDER, joints[1]);
    convertXnJointToPoint(joints, shoulders, 2);
}

void SkeletonSensor::getHeadPoint(const unsigned int i, SkeletonPoint* const head)
{
    XnSkeletonJointPosition joints;
    userG_.GetSkeletonCap().GetSkeletonJointPosition(trackedUsers_[i], XN_SKEL_HEAD, joints);
    convertXnJointToPoint(&joints, head, 1);
}

bool SkeletonSensor::isTracking()
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
    
    if (!trackedUsers_.empty())
        return TRUE;
    else
        return FALSE;
}

bool SkeletonSensor::isTracking(const unsigned int uid)
{
    for (int i = 0; i < trackedUsers_.size(); i++)
    {
        if(trackedUsers_[i] == uid)
            return TRUE;
    }

    return FALSE;
}

// set device to look for calibration pose and supply callback functions for user events
int SkeletonSensor::setCalibrationPoseCallbacks()
{
    XnCallbackHandle hUserCallbacks, hCalibrationStart, hCalibrationComplete, hPoseDetected;
    XnStatus rc = XN_STATUS_OK;

    userG_.RegisterUserCallbacks(newUserCallback, lostUserCallback, this, hUserCallbacks);
    userG_.GetSkeletonCap().RegisterToCalibrationStart(calibrationStartCallback, this, hCalibrationStart);
    userG_.GetSkeletonCap().RegisterToCalibrationComplete(calibrationCompleteCallback, this, hCalibrationComplete);

    if (needCalibrationPose_)
    {
        if (!userG_.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
        {
            put_flog(LOG_ERROR, "Pose required, but not supported by device\n");
            return -1;
        }
        rc = userG_.GetPoseDetectionCap().RegisterToPoseDetected(poseDetectedCallback, this, hPoseDetected);
        CHECK_RC(rc, "Register to Pose Detected");
        userG_.GetSkeletonCap().GetCalibrationPose((XnChar*) pose_.c_str());
    }
    
    // turn on tracking of all joints
    userG_.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
    
    return 0;
}

// print the number of poses that the connected device supports
// Kinect: 1 Pose: "Psi"
void SkeletonSensor::printAvailablePoses()
{
    XnUInt32 numPoses = userG_.GetPoseDetectionCap().GetNumberOfPoses();
    
    put_flog(LOG_DEBUG, "Number of poses: %d.\n", numPoses);
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

void XN_CALLBACK_TYPE SkeletonSensor::newUserCallback(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
    SkeletonSensor* sensor = (SkeletonSensor*) pCookie;
    put_flog(LOG_DEBUG, "New User %d\n", nId);
    // New user found
    if (sensor->getNeedCalibrationPose())
    {
        sensor->getUserGenerator()->GetPoseDetectionCap().StartPoseDetection(sensor->getPoseString(), nId);
    }
    
    // auto-calibrate user
    else
    {
        put_flog(LOG_DEBUG, "Auto-calibrating user %d.\n", nId);
        sensor->getUserGenerator()->GetSkeletonCap().RequestCalibration(nId,TRUE);
    }
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
        if (sensor->getNeedCalibrationPose())
        {
            sensor->getUserGenerator()->GetPoseDetectionCap().StartPoseDetection(sensor->getPoseString(), nId);
        }
        else
        {
            sensor->getUserGenerator()->GetSkeletonCap().RequestCalibration(nId, TRUE);
        }
    }
}

void XN_CALLBACK_TYPE SkeletonSensor::poseDetectedCallback(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie)
{
    SkeletonSensor* sensor = (SkeletonSensor*) pCookie;
    put_flog(LOG_DEBUG, "Pose %s detected for user %d\n", sensor->getPoseString(), nId);
    sensor->getUserGenerator()->GetPoseDetectionCap().StopPoseDetection(nId);
    sensor->getUserGenerator()->GetSkeletonCap().RequestCalibration(nId, TRUE);
}

SkeletonRepresentation SkeletonSensor::getAllAvailablePoints(const unsigned int UID)
{
    SkeletonRepresentation result;
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

std::vector<SkeletonRepresentation> SkeletonSensor::getAllAvailablePoints()
{
    std::vector<SkeletonRepresentation> skeletons;
    
    for(unsigned int i = 0; i < getNOTrackedUsers(); i++)
    {
        SkeletonRepresentation s = getAllAvailablePoints(trackedUsers_[i]);
        skeletons.push_back(s);
    }

    return skeletons;
}
