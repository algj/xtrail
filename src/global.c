#pragma once

#include <X11/X.h>
#include <X11/Xlib.h>

#include "point.c"

#define MASK_BACKGROUND 0
#define MASK_TRANSPARENT 0
#define MASK_SOLID 1

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct {
    int  x1;
    int  y1;
    int  x2;
    int  y2;
    char isClean;
    char wasClean;
} RenderArea;

typedef struct {
    Point p;
    int   mask;
} MouseState;

typedef struct {
    Display     *display;
    Window       window;
    unsigned int historyBufSize;
    MouseState  *historyList;
    MouseState  *state;
    unsigned int historyCur;
    unsigned int listc;
    char         hasMovedRecently;
} Mouse;

typedef struct {
    Display    *display;
    Drawable    draw;
    GC          gc;
    RenderArea *area;
    char        dither;
    float       delta;
    Mouse      *mouse;
} Canvas;