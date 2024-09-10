#pragma once

#include <X11/Xlib.h>

#include "./args.c"
#include "./global.c"
#include "./mouse.c"
#include "./render_methods.c"

// #define EffectSetFg(color) \
//     if (use_colors) XSetForeground(display, gc, color)
// #define DrawLine(point1, point2) XDrawLine(display, ref, gc, point1.x, point1.y, point2.x, point2.y)

void render(ConfigArgs *args, Canvas *c) {
    // do not render mouse if it's currently hidden
    if (c->mouse->hidden) return;

    float distTotal = 0;
    float distPrecalc[c->mouse->listc];
    for (int i = 0; i < c->mouse->listc - 2; ++i) {
        distPrecalc[i] = pointGetDistance(mouseState(c->mouse, i).p, mouseState(c->mouse, i + 1).p);
        distTotal += distPrecalc[i];
    }
    float distMax = MIN(distTotal, args->trail_length);

    if (args->type_trail) {
        float distCurrent = 0;
        for (int i = 0; i < c->mouse->listc - 1; ++i) {
            if (distCurrent >= args->trail_length) continue;
            Point p1 = mouseState(c->mouse, i).p;
            Point p2 = mouseState(c->mouse, i + 1).p;
            float dist = distPrecalc[i];
            if (dist == 0) continue;
            if (dist + distCurrent > args->trail_length) {
                float allowedLength = args->trail_length - distCurrent;
                p2 = pointInterpolate(p1, p2, 1 - (dist - allowedLength) / dist);
            }
            float g1 = distCurrent / distMax;
            distCurrent += dist;
            float g2 = distCurrent / distMax;
            if (g1 > 1 || g2 > 1 || g1 < 0 || g2 < 0) continue; // division may lead to very big numbers, so let's ignore such lines
            rTaperedGradLine(c, p1, 1 - g1, args->trail_thickness - g1 * args->trail_thickness, p2, 1 - g2,
                             args->trail_thickness - g2 * args->trail_thickness);
        }
    }

    if (args->type_dots) {
        float distCurrent = 0;
        for (int i = 0; i < c->mouse->listc; ++i) {
            if (distCurrent >= args->trail_length) continue;
            Point p = mouseState(c->mouse, i).p;
            float dist = distPrecalc[i];
            distCurrent += dist;
            float g = distCurrent / distMax;
            rRadialGrad(c, p, (1 - g) * (args->trail_thickness - 1), 0, 1 - g);
        }
    }

    // hide area around mouse
    rFCircle(c, c->mouse->state->p, args->mouse_empty_area, -1);
}