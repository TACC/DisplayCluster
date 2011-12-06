#include "JoystickThread.h"
#include "log.h"
#include "main.h"
#include "DisplayGroupJoystick.h"
#include "ContentWindowInterface.h"

JoystickThread::JoystickThread()
{
    tick1_ = tick2_ = 0;
}

JoystickThread::~JoystickThread()
{
    // close all open joysticks
    for(unsigned int i=0; i<joysticks_.size(); i++)
    {
        SDL_JoystickClose(joysticks_[i]);
    }
}

void JoystickThread::run()
{
    // we need SDL_INIT_VIDEO for events to work
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) < 0)
    {
        put_flog(LOG_ERROR, "could not initial SDL joystick subsystem");
        return;
    }
    else
    {
        put_flog(LOG_INFO, "found %i joysticks:", SDL_NumJoysticks());

        if(SDL_NumJoysticks() == 0)
        {
            return;
        }

        for(int i=0; i<SDL_NumJoysticks(); i++)
        {
            put_flog(LOG_INFO, "\t %s", SDL_JoystickName(i));
        }
    }

    // open all joysticks and create corresponding DisplayGroupJoystick objects and state objects
    for(int i=0; i<SDL_NumJoysticks(); i++)
    {
        SDL_Joystick * joystick = SDL_JoystickOpen(i);
        joysticks_.push_back(joystick);

        boost::shared_ptr<DisplayGroupJoystick> dgj(new DisplayGroupJoystick(g_displayGroupManager));
        displayGroupJoysticks_.push_back(dgj);
    }

    states_.resize(SDL_NumJoysticks());

    // setup timer to repeatedly update
    connect(&timer_, SIGNAL(timeout()), this, SLOT(updateJoysticks()));
    timer_.start(JOYSTICK_TIMER_INTERVAL);

    exec();
}

void JoystickThread::updateJoysticks()
{
    tick2_ = SDL_GetTicks();

    // update state of all joysticks
    SDL_JoystickUpdate();

    for(unsigned int i=0; i<joysticks_.size(); i++)
    {
        // see if we clicked any buttons on the window
        int button1 = SDL_JoystickGetButton(joysticks_[i], 0);

        if(button1 == 1 && states_[i].button1 == 0)
        {
            // we have a click
            states_[i].clickedWindow = displayGroupJoysticks_[i]->getContentWindowInterfaceUnderMarker();

            if(states_[i].clickedWindow != NULL)
            {
                // button dimensions
                float buttonWidth, buttonHeight;
                states_[i].clickedWindow->getButtonDimensions(buttonWidth, buttonHeight);

                // item rectangle and event position
                double x,y,w,h;
                states_[i].clickedWindow->getCoordinates(x,y,w,h);
                QRectF r(x,y,w,h);

                float markerX, markerY;
                displayGroupJoysticks_[i]->getMarker()->getPosition(markerX, markerY);
                QPointF markerPos(markerX, markerY);

                // check to see if user clicked on the close button
                if(fabs((r.x()+r.width()) - markerPos.x()) <= buttonWidth && fabs((r.y()) - markerPos.y()) <= buttonHeight)
                {
                    states_[i].clickedWindow->close();
                    states_[i].reset();
                    break;
                }

                // check to see if user clicked on the resize button
                if(fabs((r.x()+r.width()) - markerPos.x()) <= buttonWidth && fabs((r.y()+r.height()) - markerPos.y()) <= buttonHeight)
                {
                    states_[i].resizing = true;
                }

                // move to the front
                states_[i].clickedWindow->moveToFront();
            }
        }
        else if(button1 == 0 && states_[i].button1 == 1)
        {
            // unclick, reset the state
            states_[i].reset();
        }

        // save previous button state
        states_[i].button1 = button1;

        // handle motion of marker
        int axis0 = SDL_JoystickGetAxis(joysticks_[i], 0);
        int axis1 = SDL_JoystickGetAxis(joysticks_[i], 1);

        // set to 0 if below threshhold
        if(abs(axis0) < JOYSTICK_AXIS_THRESHHOLD)
        {
            axis0 = 0;
        }

        if(abs(axis1) < JOYSTICK_AXIS_THRESHHOLD)
        {
            axis1 = 0;
        }

        if(axis0 != 0 || axis1 != 0)
        {
            // elapsed time, clamping to a maximum of 0.1s
            float dt = std::min(0.1, ((float)tick2_ - (float)tick1_) / 1000.);

            // aspect ratio to scale movements correctly between left-right and up-down
            float tiledDisplayAspect = (float)g_configuration->getTotalWidth() / (float)g_configuration->getTotalHeight();

            joystickMoveMarker(i, (float)axis0 / JOYSTICK_AXIS_SCALE * dt, (float)axis1 / JOYSTICK_AXIS_SCALE * tiledDisplayAspect * dt);
        }

        // handle pan motion
        int axis2 = SDL_JoystickGetAxis(joysticks_[i], 2);
        int axis3 = SDL_JoystickGetAxis(joysticks_[i], 3);

        // set to 0 if below threshhold
        if(abs(axis2) < JOYSTICK_AXIS_THRESHHOLD)
        {
            axis2 = 0;
        }

        if(abs(axis3) < JOYSTICK_AXIS_THRESHHOLD)
        {
            axis3 = 0;
        }

        if(axis2 != 0 || axis3 != 0)
        {
            // elapsed time, clamping to a maximum of 0.1s
            float dt = std::min(0.1, ((float)tick2_ - (float)tick1_) / 1000.);

            joystickPan(i, (float)axis2 / JOYSTICK_AXIS_SCALE * dt, (float)axis3 / JOYSTICK_AXIS_SCALE * dt);
        }

        // handle zoom
        int dir = 0;

        if(SDL_JoystickGetButton(joysticks_[i], 4) == 1)
        {
            dir = 1;
        }
        else if(SDL_JoystickGetButton(joysticks_[i], 5) == 1)
        {
            dir = -1;
        }

        if(dir != 0)
        {
            joystickZoom(i, dir);
        }
    }

    tick1_ = tick2_;
}

void JoystickThread::joystickMoveMarker(int index, float dx, float dy)
{
    float x, y;
    displayGroupJoysticks_[index]->getMarker()->getPosition(x, y);

    // bound between 0 and 1
    float newX = std::max((float)0.0, std::min((float)1.0, x+dx));
    float newY = std::max((float)0.0, std::min((float)1.0, y+dy));

    displayGroupJoysticks_[index]->getMarker()->setPosition(newX, newY);

    // moving or resizing
    if(states_[index].clickedWindow != NULL)
    {
        if(states_[index].resizing == true)
        {
            double w, h;
            states_[index].clickedWindow->getSize(w, h);

            states_[index].clickedWindow->setSize(w+dx, h+dy);

            // during resize, keep marker at lower right, but just inside the window
            double x,y;
            states_[index].clickedWindow->getCoordinates(x, y, w, h);

            displayGroupJoysticks_[index]->getMarker()->setPosition(x+w-0.0001, y+h-0.0001);
        }
        else
        {
            // move the window
            double x, y;
            states_[index].clickedWindow->getPosition(x, y);

            states_[index].clickedWindow->setPosition(x+dx, y+dy);
        }
    }
}

void JoystickThread::joystickPan(int index, float dx, float dy)
{
    // get ContentWindowInterface currently underneath the marker
    boost::shared_ptr<ContentWindowInterface> cwi = displayGroupJoysticks_[index]->getContentWindowInterfaceUnderMarker();

    if(cwi != NULL)
    {
        // current center and zoom
        double centerX, centerY, zoom;
        cwi->getCenter(centerX, centerY);
        zoom = cwi->getZoom();

        // content aspect ratio, used to have a consistent left-right and up-down panning speed
        float contentAspect = 1.;

        int contentWidth, contentHeight;
        cwi->getContentDimensions(contentWidth, contentHeight);

        if(contentWidth != 0 && contentHeight != 0)
        {
            contentAspect = (float)contentWidth / (float)contentHeight;
        }

        // move the center point, scaled by the zoom factor
        cwi->setCenter(centerX + dx/zoom, centerY + dy/zoom * contentAspect);
    }
}

void JoystickThread::joystickZoom(int index, int dir)
{
    // get ContentWindowInterface currently underneath the marker
    boost::shared_ptr<ContentWindowInterface> cwi = displayGroupJoysticks_[index]->getContentWindowInterfaceUnderMarker();

    if(cwi != NULL)
    {
        // current zoom
        double zoom;
        zoom = cwi->getZoom();

        cwi->setZoom(zoom * (1. - (double)dir * JOYSTICK_ZOOM_FACTOR));
    }
}
