#ifndef SKELETON_THREAD_H
#define SKELETON_THREAD_H

#include <QThread>
#include <QtGui>
#include <map>

#include <boost/shared_ptr.hpp>

#include "SkeletonSensor.h"

class ContentWindowInterface;
class DisplayGroupJoystick;
class SkeletonSensor;

// SkeletonState: keeps track of the current state of the tracked user
// the skeleton can have 3 states:
// free movement: cursors are drawn, but no windows are active
// active window: the active window is moved with the hand and resized with two hands
// focused interaction: hand movement pans and zooms the content of active window
class SkeletonState
{
    public:
        SkeletonState();
        SkeletonState(SkeletonRepresentation& skel, const unsigned int uID);
        ~SkeletonState(){};
        
        void update(SkeletonRepresentation& skel);
        void pan(SkeletonPoint& rhand);
        void zoom(SkeletonPoint& lhand, SkeletonPoint& rhand);
        void scaleWindow(SkeletonPoint& lhand, SkeletonPoint& rhand, float threshold);
        void render();
        void drawJoints();
        void drawLimb(SkeletonPoint& p1, SkeletonPoint& p2);
        
    private:
    
        // do we have an active window?
        bool active_;
        
        // are we interacting with a focused window?
        bool focusInteraction_;
        
        // deadCursor: no movement has been detected
        bool deadCursor_;
        
        // are hands exceeding depth threshold
        bool leftHandActive_, rightHandActive_;
        
        // the current point representation of the skeleton
        SkeletonRepresentation skeletonRep_;
        
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

// SkeletonThread: recieves updates from the OpenNI device context,
// and interprets the user skeletons detected within the device FOV
class SkeletonThread : public QThread {
    Q_OBJECT

    public:

        SkeletonThread();
        ~SkeletonThread();

    protected:

        void run();

    public slots:

        void updateSkeletons();

    private:

        QTimer timer_;
        
        // the skeleton tracking sensor
        SkeletonSensor* sensor_;
        
        // the current state of each tracked user
        std::map<unsigned int, SkeletonState> states_;

};

#endif
