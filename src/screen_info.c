#pragma once

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <stdio.h>
#include <stdlib.h>

#include "./global.c"

typedef struct {
    int x;
    int y;
    int width;
    int height;
    int rate;
    int screen;
    int depth;
} ScreenInfo;

typedef struct {
    int         count;
    ScreenInfo *infos;
} ScreenList;

// we don't want to spam errors, so let's just let user know that getScreenInfo fails only once
short __getScreenHasWarnedBefore = 0;

// returns all screen info with x, y, width, height, (refresh) rate
// must be freed with freeScreenList(ScreenList *list) or free(list.infos)
ScreenList getScreenList(short quiet) {
    ScreenList screenList;
    screenList.count = 0;
    screenList.infos = NULL;
    Display            *display = XOpenDisplay(NULL);
    XRRScreenResources *screenResources = XRRGetScreenResources(display, DefaultRootWindow(display));
    if (!screenResources) {
        if (!__getScreenHasWarnedBefore && !quiet) {
            fprintf(stderr, "Error [getScreenList]: Failed to get screen resources.\n");
            __getScreenHasWarnedBefore = 1;
        }
        XCloseDisplay(display);
        return screenList;
    }

    screenList.count = screenResources->noutput;
    screenList.infos = calloc(sizeof(ScreenInfo), screenList.count);

    int                     screen = XDefaultScreen(display);
    XRRScreenConfiguration *screenConfig = XRRGetScreenInfo(display, DefaultRootWindow(display));
    if (!screenConfig) {
        if (!__getScreenHasWarnedBefore && !quiet) {
            fprintf(stderr, "Error [getScreenList]: Failed to get screen configuration.\n");
            __getScreenHasWarnedBefore = 1;
        }
        XRRFreeScreenResources(screenResources);
        XCloseDisplay(display);
        return screenList;
    }

    for (int i = 0; i < screenList.count; ++i) {
        XRROutputInfo *outputInfo = XRRGetOutputInfo(display, screenResources, screenResources->outputs[i]);
        if (!outputInfo) {
            if (!__getScreenHasWarnedBefore && !quiet) {
                fprintf(stderr, "Error [getScreenList]: Failed to get output info for screen %d.\n", i);
                __getScreenHasWarnedBefore = 1;
            }
            continue;
        }

        if (outputInfo->connection != RR_Connected) {
            XRRFreeOutputInfo(outputInfo);
            continue;
        }

        XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo(display, screenResources, outputInfo->crtc);
        if (!crtcInfo) {
            if (!__getScreenHasWarnedBefore && !quiet) {
                fprintf(stderr, "Error [getScreenList]: Failed to get crtc info for screen %d.\n", i);
                __getScreenHasWarnedBefore = 1;
            }
            XRRFreeOutputInfo(outputInfo);
            continue;
        }

        screenList.infos[i].x = crtcInfo->x;
        screenList.infos[i].y = crtcInfo->y;
        screenList.infos[i].width = crtcInfo->width;
        screenList.infos[i].height = crtcInfo->height;

        int num_rates;
        // TODO: fix
        short *rates = XRRConfigRates(screenConfig, screenResources->outputs[i], &num_rates);
        if (rates && num_rates > 0) {
            screenList.infos[i].rate = rates[0];
            XFree(rates);
        } else {
            screenList.infos[i].rate = 0;
        }

        XRRFreeCrtcInfo(crtcInfo);
        XRRFreeOutputInfo(outputInfo);
    }

    XRRFreeScreenResources(screenResources);
    XRRFreeScreenConfigInfo(screenConfig);
    XCloseDisplay(display);

    return screenList;
}

int getMaxRefreshRate(ScreenList list) {
    int rate = 1;
    if (list.count == 0) rate = 60;
    for (int i = 0; i < list.count; i++) {
        printf("%i\n", list.infos[i].rate);
        rate = MAX(rate, list.infos[i].rate);
    }
    return rate;
}

ScreenInfo getScreenSimple() {
    Display *display = XOpenDisplay(NULL);
    int      screen = DefaultScreen(display);
    int      screen_width = DisplayWidth(display, screen);
    int      screen_height = DisplayHeight(display, screen);
    XCloseDisplay(display);
    return (ScreenInfo){
        .x = 0,
        .y = 0,
        .width = screen_width,
        .height = screen_height,
        .screen = screen,
        .depth = DefaultDepth(display, screen),
    };
}