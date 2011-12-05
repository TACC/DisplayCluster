#ifndef JOYSTICK_THREAD_H
#define JOYSTICK_THREAD_H

#define JOYSTICK_AXIS_THRESHHOLD 10000 // of max range 32768
#define JOYSTICK_AXIS_SCALE (2.0 * 32768.0) // can move across the whole screen in 2s
#define JOYSTICK_ZOOM_FACTOR 0.015
#define JOYSTICK_TIMER_INTERVAL 10

#include <QtGui>
#include <QThread>
#include <SDL/SDL.h>
#include <vector>
#include <boost/shared_ptr.hpp>

class ContentWindowInterface;
class DisplayGroupJoystick;

struct JoystickState {

    JoystickState()
    {
        reset();
    }

    void reset()
    {
        button1 = 0;
        clickedWindow = boost::shared_ptr<ContentWindowInterface>();
        resizing = false;
    }

    int button1;
    boost::shared_ptr<ContentWindowInterface> clickedWindow;
    bool resizing;
};

class JoystickThread : public QThread {
    Q_OBJECT

    public:

        JoystickThread();
        ~JoystickThread();

    protected:

        void run();

    public slots:

        void updateJoysticks();

    private:

        QTimer timer_;

        int tick1_;
        int tick2_;

        std::vector<SDL_Joystick *> joysticks_;
        std::vector<boost::shared_ptr<DisplayGroupJoystick> > displayGroupJoysticks_;
        std::vector<JoystickState> states_;

        void joystickMoveMarker(int index, float dx, float dy);
        void joystickPan(int index, float dx, float dy);
        void joystickZoom(int index, int dir);
};

#endif
