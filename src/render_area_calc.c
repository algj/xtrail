#pragma once

#include <string.h>

#include "./global.c"
#include "./point.c"

/*
This is used to keep track of the area that has been used for rendering, so it could be cleaned up afterwards.
E.g., if Point is written to, it will enlarge the area that will have to be cleaned up.
*/

void inline renderAreaReset(RenderArea *a, int width, int height) {
    a->prev_x1 = a->x1;
    a->prev_y1 = a->y1;
    a->prev_x2 = a->x2;
    a->prev_y2 = a->y2;
    a->x1 = width;
    a->y1 = height;
    a->x2 = a->y2 = 0;
    a->wasClean = a->isClean;
    a->isClean = 1;
}

void inline renderAreaClaim(RenderArea *a, Point p) {
    a->isClean = 0;
    a->x1 = MIN(a->x1, p.x);
    a->y1 = MIN(a->y1, p.y);
    a->x2 = MAX(a->x2, p.x);
    a->y2 = MAX(a->y2, p.y);
}

void inline renderAreaClearImage(RenderArea *a, XImage *image) {
    const int x1 = MAX(0, a->x1);
    const int y1 = MAX(0, a->y1);
    const int x2 = MIN(a->x2, image->width);
    const int y2 = MIN(a->y2, image->height);
    if (x2 <= x1 || y2 <= y1) return;

    // RenderArea bounds are half-open. Clear every 1bpp byte intersecting [x1, x2).
    const int byteStart = x1 / 8;
    const int byteEnd = x2 / 8 + (x2 % 8 != 0);
    const size_t byteCount = (size_t)(byteEnd - byteStart);
    for (int y = y1; y < y2; ++y) {
        memset(image->data + (size_t)y * image->bytes_per_line + byteStart, 0, byteCount);
    }
}
