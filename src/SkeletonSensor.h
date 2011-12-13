#ifndef _SKELETON_SENSOR_H
#define _SKELETON_SENSOR_H

#include <XnCppWrapper.h>
#include <string>
#include <vector>

// A 3D point with the confidence of the point's location. confidence_ > 0.5 is good
struct Point
{
    float x_, y_, z_, confidence_;
};


/*
 * SkeletonSensor: A wrapper for OpenNI Skeleton tracking devices
 * 
 * Requires the OpenNI + NITE framework installation and the device driver
 * Tracks users within the device FOV, and assists in collection of user joints data
 */
 
class SkeletonSensor
{

    public:
        SkeletonSensor();
        ~SkeletonSensor();
        
        // set up the device resolution and data generators
        int initialize();
        
        // On user detection and calibration, call specified functions
        int setCalibrationPoseCallbacks();
        
        // non-blocking wait for new data on the device
        inline void waitForDeviceUpdateOnUser() { context_.WaitOneUpdateAll(userG_); }
        
        /*
         * Updates list of currently tracked users
         * Returns TRUE if there is at least on user who's skeleton is being tracked
         */
        bool isTracking();
        
        // stores the latest hand points in hands(preallocated):
        // hands[0] = left, hands[1] = right
        void getHandPoints(const unsigned int i, Point* const hands);
        
        // stores the latest elbow points in elbows
        // same convention as getHandPoints()
        void getElbowPoints(const unsigned int i, Point* const elbows);
        
        // stores the lastest arm points : hand, elbow, shoulder
        // 0 = l hand, 1 = r hand, 2 = left elbow....
        void getArmPoints(const unsigned int i, Point* const arms);
        
        // stores head points in externally managed array
        void getHeadPoint(const unsigned int i, Point* const head);
        
        // gets shoulder points
        void getShoulderPoints(const unsigned int i, Point* const shoulders);
        
        // returns unordered_map of points with keys of type <string>
        void getAllAvailablePoints(){}
        
        void setPointModeToProjective() { pointModeProjective_ = true; }
        void setPointModeToReal() { pointModeProjective_ = false; }
        void convertXnJointToPoint(XnSkeletonJointPosition* const j, Point* const p, unsigned int numPoints);
        
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
        
        bool getNeedCalibrationPose() { return needCalibrationPose_; }
        const char* getPoseString() { return pose_.c_str(); }
        void printAvailablePoses();        
            
    private:
        xn::Context        context_;
        xn::DepthGenerator depthG_;
        xn::UserGenerator  userG_;
        
        bool pointModeProjective_;
        std::string pose_;
        
        std::vector<int> trackedUsers_;
        
        float smoothingFactor_;
        
        // in older installations, a pose is needed to calibration the skeleton
        bool needCalibrationPose_;
        
        /*
         * Static callback functions for user and skeleton calibration events
         */
        static void XN_CALLBACK_TYPE newUserCallback(xn::UserGenerator& generator, XnUserID nId, void* pCookie);
        static void XN_CALLBACK_TYPE lostUserCallback(xn::UserGenerator& generator, XnUserID nId, void* pCookie);
        static void XN_CALLBACK_TYPE calibrationStartCallback(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie);
        static void XN_CALLBACK_TYPE calibrationCompleteCallback(xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus eStatus, void* pCookie);
        static void XN_CALLBACK_TYPE poseDetectedCallback(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie);
        
};

#endif
