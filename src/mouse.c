#pragma once

#include <X11/X.h>
#include <X11/Xlib.h>
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

void mouseUpdate(Mouse *mouse) {
    Window       root;
    Window       child;
    int          rootX, rootY, winX, winY;
    unsigned int mask;
    XQueryPointer(mouse->display, mouse->window, &root, &child, &rootX, &rootY, &winX, &winY, &mask);
    if (mouse->listc != mouse->historyBufSize) {
        mouse->listc++;
    }
    mouse->hasMovedRecently = mouse->listc > 0 && (mouse->historyList[mouse->historyCur].p.x != rootX ||
                                                        mouse->historyList[mouse->historyCur].p.y != rootY);
    mouse->historyCur = (mouse->historyCur + 1) % mouse->historyBufSize;
    mouse->historyList[mouse->historyCur] = (MouseState){.p = {rootX, rootY}, .mask = mask};
    mouse->state = &mouse->historyList[mouse->historyCur];
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

Mouse *mouseInit(Display *display, Window window, int historyMaxLength) {
    Mouse *mouse = malloc(sizeof(Mouse));
    historyMaxLength = MAX(2, historyMaxLength);
    mouse->display = display;
    mouse->window = window;
    mouse->historyBufSize = historyMaxLength;
    mouse->historyList = malloc(sizeof(MouseState) * mouse->historyBufSize);
    mouseReset(mouse);
    return mouse;
}