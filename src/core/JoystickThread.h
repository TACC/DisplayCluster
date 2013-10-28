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

#ifndef JOYSTICK_THREAD_H
#define JOYSTICK_THREAD_H

#define JOYSTICK_AXIS_THRESHHOLD 10000 // of max range 32768
#define JOYSTICK_AXIS_SCALE (5.0 * 32768.0) // can move across the whole screen (left-right) in 5s
#define JOYSTICK_ZOOM_FACTOR 0.02
#define JOYSTICK_SCALE_SIZE_FACTOR 0.02
#define JOYSTICK_TIMER_INTERVAL 33

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
        supported = true;
        button1 = 0;
        clickedWindow = boost::shared_ptr<ContentWindowInterface>();
        resizing = false;
    }

    // joystick name
    QString name;

    // whether this joystick is supported or not
    bool supported;

    // cursor click
    int button1;

    // currently clicked window
    boost::shared_ptr<ContentWindowInterface> clickedWindow;

    // resizing status of window
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
        void joystickScaleSize(int index, int dir);
};

#endif
