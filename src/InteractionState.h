#ifndef INTERACTION_STATE_H
#define INTERACTION_STATE_H

// the state of interaction within a window (mouse emulation)
struct InteractionState {
    double mouseX, mouseY;
    bool mouseLeft, mouseRight, mouseMiddle;

    InteractionState()
    {
        mouseX = mouseY = 0.;
        mouseLeft = mouseRight = mouseMiddle = false;
    }
};

#endif
