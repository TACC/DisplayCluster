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
        void serialize(Archive & ar, const unsigned int)
        {
            ar & x_;
            ar & y_;
            ar & z_;
            ar & confidence_;
        }
};

class Skeleton
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
        void serialize(Archive & ar, const unsigned int)
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

        // non-blocking wait for new data on the device
        inline void waitForDeviceUpdateOnUser() { context_.WaitOneUpdateAll(userG_); }

        // update vector of tracked users
        bool updateTrackedUsers();

        // return true if uid is among the tracked users
        bool isTracking(const unsigned int uid);

        // returns skeleton of specified user
        Skeleton getSkeleton(const unsigned int i);

        // returns vector of skeletons for all users
        std::vector<Skeleton> getSkeletons();

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

        // return -1 if no users, otherwise returns UID of most proximal user
        int getClosestTrackedUID();

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

        // on user detection and calibration, call specified functions
        int setCalibrationPoseCallbacks();

        // callback functions for user and skeleton calibration events
        static void XN_CALLBACK_TYPE newUserCallback(xn::UserGenerator& generator, XnUserID nId, void* pCookie);
        static void XN_CALLBACK_TYPE lostUserCallback(xn::UserGenerator& generator, XnUserID nId, void* pCookie);
        static void XN_CALLBACK_TYPE calibrationStartCallback(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie);
        static void XN_CALLBACK_TYPE calibrationCompleteCallback(xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus eStatus, void* pCookie);
        static void XN_CALLBACK_TYPE poseDetectedCallback(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie);
};

#endif
