#ifndef SKELETON_STATE_H
#define SKELETON_STATE_H

#include <QtGui>
#include <boost/shared_ptr.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>

#include "SkeletonSensor.h"

class ContentWindowInterface;
class DisplayGroupJoystick;

// SkeletonState: keeps track of the current state of the tracked user
// the skeleton can have 3 states:
// free movement: cursors are drawn, but no windows are active
// active window: the active window is moved with the hand and resized with two hands
// focused interaction: hand movement pans and zooms the content of active window
class SkeletonState
{
    public:
        SkeletonState();
        ~SkeletonState(){};

        void update(SkeletonRepresentation& skel);
        void zoom(SkeletonPoint& lhand, SkeletonPoint& rhand, float threshold);
        void pan(SkeletonPoint& rh, SkeletonPoint& rs, float maxReach);
        void scaleWindow(SkeletonPoint& lhand, SkeletonPoint& rhand, float threshold);
        void render();
        void drawJoints();
        void drawLimb(SkeletonPoint& p1, SkeletonPoint& p2);

        // are hands exceeding depth threshold
        bool leftHandActive_, rightHandActive_;
        
        // are we interacting with a focused window?
        bool focusInteraction_;

        // the current point representation of the skeleton
        SkeletonRepresentation skeletonRep_;

    protected:
            friend class boost::serialization::access;

            template<class Archive>
            void serialize(Archive & ar, const unsigned int version)
            {
                ar & leftHandActive_;
                ar & rightHandActive_;
                ar & focusInteraction_;
                ar & skeletonRep_;
            }

    private:

        // do we have an active window?
        bool active_;

        // deadCursor: no movement has been detected
        bool deadCursor_;

        // time spend hovering over inactive window
        QTime hoverTime_;

        // timeout for focuse gesture
        QTime focusTimeOut_;

        // dead cursor timeout
        QTime deadCursorTimeOut_;

        // window either being hovered over or active
        boost::shared_ptr<ContentWindowInterface> activeWindow_;

        // displayGroup for this skeleton
        boost::shared_ptr<DisplayGroupJoystick> displayGroup_;

};

#endif