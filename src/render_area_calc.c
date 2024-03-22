#pragma once

#include "./global.c"
#include "./point.c"

/*
This is used to keep track of the area that has been used for rendering, so it could be cleaned up afterwards.
E.g., if Point is written to, it will enlarge the area that will have to be cleaned up.
*/

void inline renderAreaReset(RenderArea *a, int width, int height) {
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