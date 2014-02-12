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

#ifndef TOUCH_LISTENER_H
#define TOUCH_LISTENER_H

#include <TuioListener.h>
#include <TuioClient.h>
#include <QtGui>

#define DOUBLE_CLICK_DISTANCE 0.1 // recall this is the (0,0,1,1) coordinate system
#define DOUBLE_CLICK_TIME 750 // ms

class DisplayGroupGraphicsViewProxy;

class TouchListener : public TUIO::TuioListener
{
    public:

        TouchListener();

        void addTuioObject(TUIO::TuioObject *tobj);
        void updateTuioObject(TUIO::TuioObject *tobj);
        void removeTuioObject(TUIO::TuioObject *tobj);

        void addTuioCursor(TUIO::TuioCursor *tcur);
        void updateTuioCursor(TUIO::TuioCursor *tcur);
        void removeTuioCursor(TUIO::TuioCursor *tcur);

        void refresh(TUIO::TuioTime frameTime);

    private:

        DisplayGroupGraphicsViewProxy * graphicsViewProxy_;

        TUIO::TuioClient client_;
        QPointF lastPoint_;
        QPointF cursorPos_;

        // detect double-clicks and triple-clicks
        QTime lastClickTime1_;
        QPointF lastClickPoint1_;

        QTime lastClickTime2_;
        QPointF lastClickPoint2_;
};

#endif
