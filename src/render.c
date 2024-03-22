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
    float distTotal = 0;
    float distCurrent;
    float distPrecalc[c->mouse->listc];
    // memset(&dist, 0, sizeof(dist));
    for (int i = 0; i < c->mouse->listc - 2; ++i) {
        distPrecalc[i] = pointGetDistance(mouseState(c->mouse, i).p, mouseState(c->mouse, i + 1).p);
        distTotal += distPrecalc[i];
        // if (dist_total < args->trail_length * 2) DrawLine(mouse_history[i], mouse_history[i + 1]);
    }
    float distMax = MIN(distTotal, args->trail_length);

    distCurrent = 0;
    for (int i = 0; i < c->mouse->listc - 2; ++i) {
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
        // drawLine(c, p1, p2);
        rTaperedGradLine(c, p1, 1 - g1, args->trail_thickness - g1 * args->trail_thickness, p2, 1 - g2,
                    args->trail_thickness - g2 * args->trail_thickness);
    }

    // distCurrent = 0;
    // for (int i = 1; i < c->mouse->listc; ++i) {
    //     if (distCurrent >= args->trail_length) continue;
    //     Point p = mouseState(c->mouse, i).p;
    //     float dist = distPrecalc[i];
    //     distCurrent += dist;
    //     float g = distCurrent / distMax;
    //     rRadialGrad(c, p, (1-g)*(args->trail_thickness-1), 0, 1-g);
    // }
}