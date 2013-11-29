/*********************************************************************/
/* Copyright (c) 2013, EPFL/Blue Brain Project                       */
/*                     Raphael Dumusc <raphael.dumusc@epfl.ch>       */
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

#ifndef EVENT_H
#define EVENT_H

#include <cstring>

#define UNICODE_TEXT_SIZE 4

class QDataStream;

namespace dc
{

/**
 * A user event within a window.
 *
 * Typically used to forward user inputs from a window to classes that
 * generate content for it.
 *
 * @version 1.0
 */
struct Event
{
    /**
     * The EventType enum defines the different types of interaction.
     * @version 1.0
     */
    enum EventType
    {
        EVT_NONE,
        EVT_PRESS,
        EVT_RELEASE,
        EVT_CLICK,
        EVT_DOUBLECLICK,
        EVT_MOVE,
        EVT_WHEEL,
        EVT_SWIPE_LEFT,
        EVT_SWIPE_RIGHT,
        EVT_SWIPE_UP,
        EVT_SWIPE_DOWN,
        EVT_CLOSE,
        EVT_KEY_PRESS,
        EVT_KEY_RELEASE,
        EVT_VIEW_SIZE_CHANGED
    };

    /** The type of event */
    EventType type;

    /** @name Mouse and touch events */
    /*@{*/
    double mouseX;    /**< Normalized X mouse/touch position relative to the window */
    double mouseY;    /**< Normalized Y mouse/touch position relative to the window */
    double dx;        /**< Normalized horizontal delta for scroll events / delta in pixels for wheel events. */
    double dy;        /**< Normalized vertical delta for scroll events / delta in pixels for wheel events. */
    bool mouseLeft;   /**< The state of the left mouse button (pressed=true) */
    bool mouseRight;  /**< The state of the right mouse button (pressed=true) */
    bool mouseMiddle; /**< The state of the middle mouse button (pressed=true) */
    /*@}*/

    /** @name Keyboard events */
    /*@{*/
    int key;         /**< The key code, see QKeyEvent::key() */
    int modifiers;   /**< The keyboard modifiers, see QKeyEvent::modifiers() */
    char text[UNICODE_TEXT_SIZE];   /**< Carries unicode for key, see QKeyEvent::text() */
    /*@}*/

    /** Construct a new event. @version 1.0 */
    Event()
        : type(EVT_NONE)
        , mouseX(0)
        , mouseY(0)
        , dx(0)
        , dy(0)
        , mouseLeft(false)
        , mouseRight(false)
        , mouseMiddle(false)
        , key(0)
        , modifiers(0)
    {}

    /** The size of the QDataStream serialized output. */
    static const size_t serializedSize;
};

/** Serialization for network, where sizeof(Event) can differ between compilers. */
QDataStream& operator<<(QDataStream& out, const Event& event);
QDataStream& operator>>(QDataStream& in, Event& event);

}

#endif
