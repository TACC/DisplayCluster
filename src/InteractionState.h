#ifndef INTERACTION_STATE_H
#define INTERACTION_STATE_H

#include <string>

// the state of interaction within a window (mouse emulation)
struct InteractionState {
    enum EventType
    {
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
        EVT_VIEW_SIZE_CHANGED,
        EVT_NONE
    };

    double mouseX, mouseY, dx, dy;
    bool mouseLeft, mouseRight, mouseMiddle;
    EventType type;
    int key;
    int modifiers;
    char text[4]; // carries unicode for key, see QKeyEvent::text()

    InteractionState()
    {
        mouseX = mouseY = dx = dy = 0.;
        mouseLeft = mouseRight = mouseMiddle = false;
        type = EVT_NONE;
        key = modifiers = 0;
    }
};

#endif
