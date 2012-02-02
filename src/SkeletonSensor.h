#ifndef SKELETON_SENSOR_H
#define SKELETON_SENSOR_H

#include <XnCppWrapper.h>
#include <string>
#include <vector>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

// A 3D point with the confidence of the point's location. confidence_ > 0.5 is good
class SkeletonPoint
{
    public:
        float x_, y_, z_, confidence_;

    protected:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & x_;
            ar & y_;
            ar & z_;
            ar & confidence_;
        }
};

class SkeletonRepresentation
{
    public:
        SkeletonPoint head_;
        SkeletonPoint neck_;
        SkeletonPoint rightShoulder_;
        SkeletonPoint leftShoulder_;
        SkeletonPoint rightElbow_;
        SkeletonPoint leftElbow_;
        SkeletonPoint rightHand_;
        SkeletonPoint leftHand_;
        SkeletonPoint rightHip_;
        SkeletonPoint leftHip_;
        SkeletonPoint rightKnee_;
        SkeletonPoint leftKnee_;
        SkeletonPoint rightFoot_;
        SkeletonPoint leftFoot_;
        SkeletonPoint torso_;

    protected:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar & head_;
            ar & neck_;
            ar & rightShoulder_;
            ar & leftShoulder_;
            ar & rightElbow_;
            ar & leftElbow_;
            ar & rightHand_;
            ar & leftHand_;
            ar & rightHip_;
            ar & leftHip_;
            ar & rightKnee_;
            ar & leftKnee_;
            ar & rightFoot_;
            ar & leftFoot_;
            ar & torso_;
        }
};

// SkeletonSensor: A wrapper for OpenNI Skeleton tracking devices
//  
// Requires the OpenNI + NITE framework installation and the device driver
// Tracks users within the device FOV, and assists in collection of user joints data
 
class SkeletonSensor
{

    public:
        SkeletonSensor();
        ~SkeletonSensor();
        
        // set up the device resolution and data generators
        int initialize();
        
        // on user detection and calibration, call specified functions
        int setCalibrationPoseCallbacks();
        
        // non-blocking wait for new data on the device
        inline void waitForDeviceUpdateOnUser() { context_.WaitOneUpdateAll(userG_); }
        
        // update vector of tracked users
        bool updateTrackedUsers();
        
        // updates list of currently tracked users
        // returns TRUE if there is at least one user who's skeleton is being tracked
        bool isTracking();
        
        // return true if uid is among the tracked users
        bool isTracking(const unsigned int uid);
        
        // stores the latest hand points in hands(preallocated):
        // hands[0] = left, hands[1] = right
        void getHandPoints(const unsigned int i, SkeletonPoint* const hands);
        
        // stores the latest elbow points in elbows
        // same convention as getHandPoints()
        void getElbowPoints(const unsigned int i, SkeletonPoint* const elbows);
        
        // stores the lastest arm points : hand, elbow, shoulder
        // 0 = l hand, 1 = r hand, 2 = left elbow....
        void getArmPoints(const unsigned int i, SkeletonPoint* const arms);
        
        // stores head points in externally managed array
        void getHeadPoint(const unsigned int i, SkeletonPoint* const head);
        
        // gets shoulder points
        void getShoulderPoints(const unsigned int i, SkeletonPoint* const shoulders);
        
        // returns skeleton of specified user
        SkeletonRepresentation getAllAvailablePoints(const unsigned int i);
        
        // returns vector of skeletons for all users
        std::vector<SkeletonRepresentation> getAllAvailablePoints();
        
        void setPointModeToProjective() { pointModeProjective_ = true; }
        void setPointModeToReal() { pointModeProjective_ = false; }
        void convertXnJointToPoint(XnSkeletonJointPosition* const j, SkeletonPoint* const p, unsigned int numPoints);
        
        // set the smoothing factor
        inline void setSmoothing(const float smoothingF)
        {
            smoothingFactor_ = smoothingF;
        }
        
        void startGeneratingAll() { context_.StartGeneratingAll(); }
        
        xn::UserGenerator* getUserGenerator() { return &userG_; }
        xn::DepthGenerator* getDepthGenerator() { return &depthG_; }
        void getDepthMetaData(xn::DepthMetaData& depthMD) { depthG_.GetMetaData(depthMD); }
        void getDepthSceneMetaData(xn::SceneMetaData& sceneMD) { userG_.GetUserPixels(0, sceneMD); }
        
        unsigned int getNOTrackedUsers() { return trackedUsers_.size(); }
        unsigned int getUID(int i) { return trackedUsers_[i]; }
        void addTrackedUser(const int uID) { trackedUsers_.push_back(uID); }
        void removeTrackedUser(const int uID);
        
        // return -1 if no users, otherwise returns UID of most proximal user
        int getClosestTrackedUID();
        
        bool getNeedCalibrationPose() { return needCalibrationPose_; }
        const char* getPoseString() { return pose_.c_str(); }
        void printAvailablePoses();        
            
    private:
        xn::Context        context_;
        xn::DepthGenerator depthG_;
        xn::UserGenerator  userG_;
        
        bool pointModeProjective_;
        std::string pose_;
        
        std::vector<unsigned int> trackedUsers_;
        
        float smoothingFactor_;
        
        // in older installations, a pose is needed to calibration the skeleton
        bool needCalibrationPose_;
        
        // callback functions for user and skeleton calibration events
        static void XN_CALLBACK_TYPE newUserCallback(xn::UserGenerator& generator, XnUserID nId, void* pCookie);
        static void XN_CALLBACK_TYPE lostUserCallback(xn::UserGenerator& generator, XnUserID nId, void* pCookie);
        static void XN_CALLBACK_TYPE calibrationStartCallback(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie);
        static void XN_CALLBACK_TYPE calibrationCompleteCallback(xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus eStatus, void* pCookie);
        static void XN_CALLBACK_TYPE poseDetectedCallback(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie);
};

#endif
