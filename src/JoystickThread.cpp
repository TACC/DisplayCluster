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

#include "JoystickThread.h"
#include "log.h"
#include "main.h"
#include "DisplayGroupJoystick.h"
#include "ContentWindowInterface.h"

JoystickThread::JoystickThread()
{
    moveToThread(this);

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
    put_flog(LOG_INFO, "found %i joysticks:", SDL_NumJoysticks());

    if(SDL_NumJoysticks() == 0)
    {
        return;
    }

    for(int i=0; i<SDL_NumJoysticks(); i++)
    {
        put_flog(LOG_INFO, "\t %s", SDL_JoystickName(i));
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

    for(unsigned int index=0; index<joysticks_.size(); index++)
    {
        // save name of joystick
        if(states_[index].name.isEmpty() == true)
        {
            states_[index].name = QString(SDL_JoystickName(index));
        }

        // make sure we have a supported joystick
        if(states_[index].supported == false)
        {
            break;
        }

        // get updated state

        // cursor movement up-down and left-right: axis1, axis2
        // panning up-down and left-right: axis3, axis4
        // cursor click: button1
        // zoom out and in: button2, button3
        // scale down and up: button4, button5
        int axis1, axis2, axis3, axis4;
        int button1, button2, button3, button4, button5;

        if(states_[index].name.contains("Logitech") == true)
        {
            axis1 = SDL_JoystickGetAxis(joysticks_[index], 1);
            axis2 = SDL_JoystickGetAxis(joysticks_[index], 0);

            axis3 = SDL_JoystickGetAxis(joysticks_[index], 3);
            axis4 = SDL_JoystickGetAxis(joysticks_[index], 2);

            button1 = SDL_JoystickGetButton(joysticks_[index], 0);

            button2 = SDL_JoystickGetButton(joysticks_[index], 4);
            button3 = SDL_JoystickGetButton(joysticks_[index], 5);
            button4 = SDL_JoystickGetButton(joysticks_[index], 6);
            button5 = SDL_JoystickGetButton(joysticks_[index], 7);
        }
        else if(states_[index].name.contains("Xbox") == true)
        {
            axis1 = SDL_JoystickGetAxis(joysticks_[index], 1);
            axis2 = SDL_JoystickGetAxis(joysticks_[index], 0);

            axis3 = SDL_JoystickGetAxis(joysticks_[index], 4);
            axis4 = SDL_JoystickGetAxis(joysticks_[index], 3);

            button1 = SDL_JoystickGetButton(joysticks_[index], 2);

            button2 = SDL_JoystickGetButton(joysticks_[index], 4);
            button3 = SDL_JoystickGetButton(joysticks_[index], 5);

            button4 = (SDL_JoystickGetAxis(joysticks_[index], 2) > JOYSTICK_AXIS_THRESHHOLD) ? 1 : 0;
            button5 = (SDL_JoystickGetAxis(joysticks_[index], 5) > JOYSTICK_AXIS_THRESHHOLD) ? 1 : 0;
        }
        else
        {
            put_flog(LOG_ERROR, "unknown joystick type %s", states_[index].name.toStdString().c_str());

            states_[index].supported = false;

            break;
        }

        // axis threshholds
        if(abs(axis1) < JOYSTICK_AXIS_THRESHHOLD)
        {
            axis1 = 0;
        }

        if(abs(axis2) < JOYSTICK_AXIS_THRESHHOLD)
        {
            axis2 = 0;
        }

        if(abs(axis3) < JOYSTICK_AXIS_THRESHHOLD)
        {
            axis3 = 0;
        }

        if(abs(axis4) < JOYSTICK_AXIS_THRESHHOLD)
        {
            axis4 = 0;
        }

        // see if we clicked any buttons on the window
        if(button1 == 1 && states_[index].button1 == 0)
        {
            // we have a click
            states_[index].clickedWindow = displayGroupJoysticks_[index]->getContentWindowInterfaceUnderMarker();

            if(states_[index].clickedWindow != NULL)
            {
                // button dimensions
                float buttonWidth, buttonHeight;
                states_[index].clickedWindow->getButtonDimensions(buttonWidth, buttonHeight);

                // item rectangle and event position
                double x,y,w,h;
                states_[index].clickedWindow->getCoordinates(x,y,w,h);
                QRectF r(x,y,w,h);

                float markerX, markerY;
                displayGroupJoysticks_[index]->getMarker()->getPosition(markerX, markerY);
                QPointF markerPos(markerX, markerY);

                // check to see if user clicked on the close button
                if(fabs((r.x()+r.width()) - markerPos.x()) <= buttonWidth && fabs((r.y()) - markerPos.y()) <= buttonHeight)
                {
                    states_[index].clickedWindow->close();
                    states_[index].reset();
                    break;
                }

                // check to see if user clicked on the resize button
                if(fabs((r.x()+r.width()) - markerPos.x()) <= buttonWidth && fabs((r.y()+r.height()) - markerPos.y()) <= buttonHeight)
                {
                    states_[index].resizing = true;
                }

                // move to the front
                states_[index].clickedWindow->moveToFront();
            }
        }
        else if(button1 == 0 && states_[index].button1 == 1)
        {
            // unclick, reset the state
            states_[index].reset();
        }

        // save previous button state
        states_[index].button1 = button1;

        // handle motion of marker
        if(axis1 != 0 || axis2 != 0)
        {
            // elapsed time, clamping to a maximum of 0.1s
            float dt = std::min(0.1, ((float)tick2_ - (float)tick1_) / 1000.);

            // aspect ratio to scale movements correctly between left-right and up-down
            float tiledDisplayAspect = (float)g_configuration->getTotalWidth() / (float)g_configuration->getTotalHeight();

            joystickMoveMarker(index, (float)axis2 / JOYSTICK_AXIS_SCALE * dt, (float)axis1 / JOYSTICK_AXIS_SCALE * tiledDisplayAspect * dt);
        }

        // handle pan motion
        if(axis3 != 0 || axis4 != 0)
        {
            // elapsed time, clamping to a maximum of 0.1s
            float dt = std::min(0.1, ((float)tick2_ - (float)tick1_) / 1000.);

            joystickPan(index, (float)axis4 / JOYSTICK_AXIS_SCALE * dt, (float)axis3 / JOYSTICK_AXIS_SCALE * dt);
        }

        // handle zoom
        int dir = 0;

        if(button2 == 1)
        {
            dir = 1;
        }
        else if(button3 == 1)
        {
            dir = -1;
        }

        if(dir != 0)
        {
            joystickZoom(index, dir);
        }

        // handle scaling
        dir = 0;

        if(button4 == 1)
        {
            dir = -1;
        }
        else if(button5 == 1)
        {
            dir = 1;
        }

        if(dir != 0)
        {
            joystickScaleSize(index, dir);
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

void JoystickThread::joystickScaleSize(int index, int dir)
{
    // get ContentWindowInterface currently underneath the marker
    boost::shared_ptr<ContentWindowInterface> cwi = displayGroupJoysticks_[index]->getContentWindowInterfaceUnderMarker();

    if(cwi != NULL)
    {
        cwi->scaleSize(1. + (double)dir * JOYSTICK_SCALE_SIZE_FACTOR);
    }
}
