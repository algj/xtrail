#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/shape.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

#include "./args.c"
#include "./global.c"
#include "./mouse.c"
#include "./mouse_thread.c"
#include "./render.c"
#include "./render_area_calc.c"
#include "./screen_info.c"
#include "./time.c"

// x11
Display   *display = NULL;
Window     window;
Pixmap     maskPixmap;
GC         maskGC = NULL;
XImage    *image;
ScreenInfo screen;
ConfigArgs args;
RenderArea area;

double frameTimer;
double frameDelta;
int    detected_refresh_rate = 60;

// how many rendered frames shall be waited until checking
// screen resolution, refresh rate and other things
#define UPDATE_WAIT_TICKS 10000

void unloadMask() {
    if (maskGC != NULL) {
        if (image->data != NULL) free(image->data);
        XFreeGC(display, maskGC);
        XFreePixmap(display, maskPixmap);
    }
}
void unloadEverything() {
    unloadMask();
    if (args.mouse_separate_thread) {
        pthread_cancel(mouseThreadID);
        XCloseDisplay(mouse->display);
    }
    mouseFree(mouse);
    if (display) XCloseDisplay(display);
}

void flushMask() {
    // XPutImage(display, maskPixmap, maskGC, image, 0, 0, 0, 0, screen.width, screen.height);
    int x = MAX(0, MIN(area.x1, area.prev_x1));
    int y = MAX(0, MIN(area.y1, area.prev_y1));
    int w = MIN(screen.width, MAX(area.x2, area.prev_x2) - x);
    int h = MIN(screen.height, MAX(area.y2, area.prev_y2) - y);
    XPutImage(display, maskPixmap, maskGC, image, x, y, x, y, w, h);
    XShapeCombineMask(display, window, ShapeBounding, 0, 0, maskPixmap, ShapeSet);
}

void windowReset() {
    unloadMask();

    // create a pixmap for the mask
    // unfortunately without compositor you cannot have 8bit alpha channel
    // we "solve" this by dithering... ;-;
    // this will work, but someone could make it have an actual alpha channel
    maskPixmap = XCreatePixmap(display, window, screen.width, screen.height, 1);
    maskGC = XCreateGC(display, maskPixmap, 0, NULL);

    image = XCreateImage(display, DefaultVisual(display, DefaultScreen(display)), 1, ZPixmap, 0, NULL, screen.width,
                         screen.height, 32, 0);

    // alloc memory for img
    // we add a bit padding bytes, since we clean up the working area with some optimizations
    image->data = malloc(image->height * image->bytes_per_line + 128);
    if (image->data == NULL) {
        fprintf(stderr, "Error [windowReset]: failed to malloc for image data\n");
        unloadEverything();
        exit(EXIT_FAILURE);
    }

    // move if x11 dimensions have changed
    XResizeWindow(display, window, screen.width, screen.height);
    XMoveWindow(display, window, 0, 0);

    // make window transparent before showing it (avoids flashing when launches)
    Pixmap emptyPixmap = XCreatePixmap(display, window, screen.width, screen.height, 1);
    GC     emptyGC = XCreateGC(display, emptyPixmap, 0, NULL);
    XShapeCombineMask(display, window, ShapeBounding, 0, 0, emptyPixmap, ShapeSet);
    XShapeCombineMask(display, window, ShapeInput, 0, 0, emptyPixmap, ShapeSet);
    XFreeGC(display, emptyGC);
    XFreePixmap(display, emptyPixmap);

    XFlush(display);

    renderAreaReset(&area, screen.width, screen.height);
}

void windowSetup() {
    window = XCreateSimpleWindow(display, RootWindow(display, screen.screen), 0, 0, screen.width, screen.height, 0, 0x000000,
                                 args.color);

    // Let's change this to desktop type to avoid compositors touching it
    Atom desktopAtom = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    XChangeProperty(display, window, XInternAtom(display, "_NET_WM_WINDOW_TYPE", False), XA_ATOM, 32, PropModeReplace,
                    (unsigned char *)&desktopAtom, 1);

    // Blank cursor
    Pixmap   pixmap = XCreatePixmap(display, window, 1, 1, 1);
    XColor   blackColor;
    Colormap colormap = DefaultColormap(display, screen.screen);
    XAllocNamedColor(display, colormap, "black", &blackColor, &blackColor);
    Cursor transparentCursor = XCreatePixmapCursor(display, pixmap, pixmap, &blackColor, &blackColor, 0, 0);
    XDefineCursor(display, window, transparentCursor);

    // Set window attributes
    XSetWindowAttributes attrs;
    attrs.override_redirect = True;        // Override redirect to prevent window manager interaction
    attrs.event_mask = PointerMotionMask;  // Handle mouse motion events
    attrs.do_not_propagate_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                                  Button1MotionMask | Button2MotionMask | Button3MotionMask | Button4MotionMask |
                                  Button5MotionMask | ButtonMotionMask;
    XChangeWindowAttributes(display, window, CWOverrideRedirect | CWEventMask | CWDontPropagate, &attrs);
    XSelectInput(display, window, ExposureMask | KeyPressMask);

    windowReset();
    frameTimer = getCurrentTimeMsDouble();

    XMapWindow(display, window);

    XFlush(display);
}

void render_task() {
    // clone mouse so it doesn't get changed mid-update
    Mouse *mouseCopy = mouse;
    if (args.mouse_separate_thread) mouseCopy = mouseClone();

    Canvas canvas = {
        .display = display,
        .draw = maskPixmap,
        .gc = maskGC,
        .area = &area,
        .image = image,
        .dither = args.dither,
        .delta = frameDelta,
        .mouse = mouseCopy,
    };

    // here we go, let's do some fancy shit
    render(&args, &canvas);

    if (args.mouse_separate_thread) mouseFree(mouseCopy);

    // let's flush what we've rendered
    if (!area.wasClean || !area.isClean) {
        flushMask();
        XFlush(display);
    }
    if (!area.isClean) {
        // clean the area for next frame
        renderAreaClearImage(&area, image);
    }

    renderAreaReset(&area, screen.width, screen.height);
}

int  ticksCounterForUpdates = 0;
void updateInfrequently() {
    ScreenInfo screen_new = getScreenSimple();
    if (args.refresh_rate == -1) {
        ScreenList list = getScreenList(0);
        if (list.infos) {
            // TODO: fix
            // detected_refresh_rate = getMaxRefreshRate(list);
            free(list.infos);
        }
    }
    if (screen.x != screen_new.x || screen.y != screen_new.y || screen.width != screen_new.width ||
        screen.height != screen_new.height) {
        // if screen dimensions have changed
        screen = screen_new;
        windowReset();
    }
}

int main(int argc, char *argv[]) {
    args = parseArgs(argc, argv);
    if (args.quit) return 0;

    display = XOpenDisplay(NULL);
    if (display == NULL) {
        fprintf(stderr, "Error [main]: Unable to open display; are you sure you're using x11?\n");
        return 1;
    }

    // init mouse
    if (args.mouse_separate_thread) {
        mouseRefreshRate = args.mouse_refresh_rate;
        mouseInitThread(args.pos_count, args.mouse_interpolation_factor);
    } else {
        mouse = mouseInit(display, DefaultRootWindow(display), args.pos_count, args.mouse_interpolation_factor);
    }

    // init screen
    screen = getScreenSimple();

    // init window
    renderAreaReset(&area, screen.width, screen.height);
    windowSetup();

    // main loop
    while (1) {
        double timerNew = getCurrentTimeMsDouble();
        frameDelta = timerNew - frameTimer;
        frameTimer = timerNew;

        // for resource intensive tasks
        if (ticksCounterForUpdates == 0) {
            ticksCounterForUpdates = UPDATE_WAIT_TICKS;
            updateInfrequently();
        }
        ticksCounterForUpdates--;

        // update mouse position if mouse thread is not running
        if (!args.mouse_separate_thread) mouseUpdate(mouse);

        // grab the events that happened to the window
        XEvent event;
        while (XPending(display)) {
            XNextEvent(display, &event);
            switch (event.type) {
                // --
            }
        }

        // do the render preparations and the rendering itself
        render_task();

        // keep trails on top of other windows (honestly this should be somewhere else)
        XRaiseWindow(display, window);

        // calculate time taken
        double elapsedTime = getCurrentTimeMsDouble() - frameTimer;

        // fps counter for debugging
        // printf("%2f fps\n", 1000 / elapsedTime);

        // how much time to sleep to keep refresh rate accurate
        double targetFrameTime = 1000.0 / (args.refresh_rate == -1 ? detected_refresh_rate : args.refresh_rate);
        double remainingTime = targetFrameTime - elapsedTime;

        // Adjust sleeping time if rendering took too long
        if (remainingTime > 0) {
            if (args.mouse_separate_thread) {
                usleep(remainingTime * 1000);
            } else {
                double mouseUpdateInterval = 1000.0 / args.mouse_refresh_rate;
                int    subframe_mouse_pooling = args.mouse_separate_thread ? 1 : args.mouse_refresh_rate / args.refresh_rate;
                for (int i = 0; i < subframe_mouse_pooling; i++) {
                    mouseUpdate(mouse);
                    usleep((mouseUpdateInterval / subframe_mouse_pooling) * 1000);
                }
            }
        }
    }

    return 0;
}
