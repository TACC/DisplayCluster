#ifndef SKELETON_THREAD_H
#define SKELETON_THREAD_H

#include <QThread>
#include <QtGui>

class SkeletonSensor;

// SkeletonThread: Recieves updates from the OpenNI device context,
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
        
        SkeletonSensor* sensor_;

        /* Greg Stuff
        int tick1_;
        int tick2_;

        
        std::vector<boost::shared_ptr<DisplayGroupJoystick> > displayGroupJoysticks_;
        std::vector<JoystickState> states_;

        void joystickMoveMarker(int index, float dx, float dy);
        void joystickPan(int index, float dx, float dy);
        void joystickZoom(int index, int dir);
        */
};

#endif