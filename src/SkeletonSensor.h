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
#include <vector>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

// A 3D point with the confidence of the point's location. confidence_ > 0.5 is good
struct SkeletonPoint
{
    float x, y, z, confidence;

    protected:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int)
        {
            ar & x;
            ar & y;
            ar & z;
            ar & confidence;
        }
};

struct Skeleton
{
    SkeletonPoint head;
    SkeletonPoint neck;
    SkeletonPoint rightShoulder;
    SkeletonPoint leftShoulder;
    SkeletonPoint rightElbow;
    SkeletonPoint leftElbow;
    SkeletonPoint rightHand;
    SkeletonPoint leftHand;
    SkeletonPoint rightHip;
    SkeletonPoint leftHip;
    SkeletonPoint rightKnee;
    SkeletonPoint leftKnee;
    SkeletonPoint rightFoot;
    SkeletonPoint leftFoot;
    SkeletonPoint torso;

    protected:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int)
        {
            ar & head;
            ar & neck;
            ar & rightShoulder;
            ar & leftShoulder;
            ar & rightElbow;
            ar & leftElbow;
            ar & rightHand;
            ar & leftHand;
            ar & rightHip;
            ar & leftHip;
            ar & rightKnee;
            ar & leftKnee;
            ar & rightFoot;
            ar & leftFoot;
            ar & torso;
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
        void waitForDeviceUpdateOnUser();

        // update vector of tracked users
        void updateTrackedUsers();

        // return true if UID is among the tracked users
        bool isTracking(const unsigned int uid);

        // returns skeleton of specified user
        Skeleton getSkeleton(const unsigned int uid);

        // returns vector of skeletons for all users
        std::vector<Skeleton> getSkeletons();

        // get number of tracked users
        unsigned int getNumTrackedUsers();

        // map tracked user index to UID
        unsigned int getUID(const unsigned int index);

        // change point mode
        void setPointModeToProjective();
        void setPointModeToReal();

    private:
        xn::Context context_;
        xn::DepthGenerator depthG_;
        xn::UserGenerator userG_;

        std::vector<unsigned int> trackedUsers_;

        bool pointModeProjective_;

        // on user detection and calibration, call specified functions
        int setCalibrationPoseCallbacks();

        // joint to point conversion, considers point mode
        void convertXnJointsToPoints(XnSkeletonJointPosition* const j, SkeletonPoint* const p, unsigned int numPoints);

        // callback functions for user and skeleton calibration events
        static void XN_CALLBACK_TYPE newUserCallback(xn::UserGenerator& generator, XnUserID nId, void* pCookie);
        static void XN_CALLBACK_TYPE lostUserCallback(xn::UserGenerator& generator, XnUserID nId, void* pCookie);
        static void XN_CALLBACK_TYPE calibrationStartCallback(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie);
        static void XN_CALLBACK_TYPE calibrationCompleteCallback(xn::SkeletonCapability& capability, XnUserID nId, XnCalibrationStatus eStatus, void* pCookie);
        static void XN_CALLBACK_TYPE poseDetectedCallback(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie);
};

#endif
