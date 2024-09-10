#pragma once

#include <X11/X.h>
#include <X11/Xlib.h>

#include "point.c"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

typedef struct {
    int  x1;
    int  y1;
    int  x2;
    int  y2;
    int  prev_x1;
    int  prev_y1;
    int  prev_x2;
    int  prev_y2;
    char isClean;
    char wasClean;
} RenderArea;

typedef struct {
    Point p;
    FPoint fp;
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
    int          interpolationFactor;
    char         hidden;
    unsigned int loopIndex;
} Mouse;

typedef struct {
    Display    *display;
    Drawable    draw;
    GC          gc;
    XImage     *image;
    RenderArea *area;
    char        dither;
    float       delta;
    Mouse      *mouse;
} Canvas;