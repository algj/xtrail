#pragma once

#include <X11/X.h>
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>

#include "./global.c"

/*
Example Usage:

MouseDetails *mouse = mouseInit(display, window, 256);
while(1){
    mouseUpdate(mouse);
    MouseState *mv;
    mouseForHistory(mouse, index, mv) {
        if(mouseButtonClicked(mouse, 1)){
            printf("Clicked at pos: %i, %i\n", mv->p.x, mv->p.y);
        }
    }
}
*/

// first = most recent, last = oldest
// for performance reasons, don't forget to use `break` to quit the loop
#define mouseForHistory(mouse, mouseEl) \
    for (int index = 0; (mouseEl = &mouseState(mouse, index)), index < mouse->listc; index++)

#define mouseState(mouse, index) \
    (mouse->historyList[(mouse->historyCur - (index) + mouse->historyBufSize) % mouse->historyBufSize])
// #define mouseStateExists(mouse, index) (mouse->listc < index)
#define mouseButtonPressed(mouse, index, button) ((mouseState(mouse, index).mask & mouseBtnMasks[button]) != 0)
#define mouseButtonClicked(mouse, index, button) \
    (mouseButtonPressed(mouse, (index), button) && !mouseButtonPressed(mouse, 1 + (index), button))
#define mouseButtonReleased(mouse, index, button) \
    (!mouseButtonPressed(mouse, (index), button) && mouseButtonPressed(mouse, 1 + (index), button))

const int mouseBtnMasks[6] = {
    Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask,  // check all buttons
    Button1Mask,
    Button2Mask,
    Button3Mask,
    Button4Mask,
    Button5Mask,
};

#define MOUSE_INCREASE \
    do { \
        (mouse->historyCur = (mouse->historyCur + 1) % mouse->historyBufSize); \
        if (mouse->listc != mouse->historyBufSize) mouse->listc++; \
    } while (0);
#define MOUSE_DECREASE \
    do { \
        (mouse->historyCur = (mouse->historyCur - 1 + mouse->historyBufSize) % mouse->historyBufSize); \
        if (mouse->listc > 0) mouse->listc--; \
    } while (0);
#define MOUSE_CURRENT (mouse->historyList[mouse->historyCur])

void mouseUpdate(Mouse *mouse) {
    Window       root;
    Window       child;
    int          rootX, rootY, winX, winY;
    unsigned int mask;
    XQueryPointer(mouse->display, mouse->window, &root, &child, &rootX, &rootY, &winX, &winY, &mask);
    MouseState mcurr = (MouseState){
        .fp = {rootX, rootY},
        .p = {rootX, rootY},
        .mask = mask,
    };
    // TODO: interpolate
    MOUSE_INCREASE;
    MOUSE_CURRENT = mcurr;
    mouse->state = &MOUSE_CURRENT;
}

void mouseReset(Mouse *mouse) {
    mouse->historyCur = mouse->historyBufSize - 1;
    mouse->listc = 0;
    mouseUpdate(mouse);
}

void mouseFree(Mouse *mouse) {
    free(mouse->historyList);
    free(mouse);
}

Mouse *mouseInit(Display *display, Window window, int historyMaxLength, int interpolationFactor) {
    Mouse *mouse = malloc(sizeof(Mouse));
    historyMaxLength = MAX(2, historyMaxLength) * MAX(1, 1+interpolationFactor);
    mouse->interpolationFactor = interpolationFactor;
    mouse->display = display;
    mouse->window = window;
    mouse->historyBufSize = historyMaxLength;
    mouse->historyList = malloc(sizeof(MouseState) * mouse->historyBufSize);
    mouseReset(mouse);
    return mouse;
}