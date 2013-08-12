#ifndef INTERACTION_STATE_H
#define INTERACTION_STATE_H

// the state of interaction within a window (mouse emulation)
struct InteractionState {
    enum EventType
    {
        EVT_PRESS,
        EVT_RELEASE,
        EVT_DOUBLECLICK,
        EVT_MOVE,
        EVT_WHEEL,
        EVT_CLOSE,
        EVT_KEY_PRESS,
        EVT_KEY_RELEASE,
        EVT_NONE
    };

    double mouseX, mouseY, dx, dy;
    bool mouseLeft, mouseRight, mouseMiddle;
    EventType type;
    int key;

    InteractionState()
    {
        mouseX = mouseY = dx = dy = 0.;
        mouseLeft = mouseRight = mouseMiddle = false;
        type = EVT_NONE;
    }
};

#endif
