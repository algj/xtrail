#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mouse.c"
#include "time.c"

Mouse          *mouse;
char            mouseThreaded = 0;
int             mouseRefreshRate = 480;
pthread_t       mouseThreadID;
pthread_mutex_t mouseMutex = PTHREAD_MUTEX_INITIALIZER;

void mouseThreadUse() {
    if (mouseThreaded) pthread_mutex_lock(&mouseMutex);
}
void mouseThreadRelease() {
    if (mouseThreaded) pthread_mutex_unlock(&mouseMutex);
}

Mouse *mouseClone() {
    mouseThreadUse();

    Mouse *mouse_cp = malloc(sizeof(Mouse));
    memcpy(mouse_cp, mouse, sizeof(Mouse));
    mouse_cp->historyList = malloc(sizeof(MouseState) * mouse->historyBufSize);
    memcpy(mouse_cp->historyList, mouse->historyList, sizeof(MouseState) * mouse->historyBufSize);
    mouse_cp->state = &mouseState(mouse_cp, 0);

    mouseThreadRelease();
    return mouse_cp;
}

void *mouseThreadLoop(void *arg) {
    mouseThreaded = 1;
    double startTime, endTime, elapsedTime;

    while (1) {
        double targetFrameTime = 1000.0 / mouseRefreshRate;
        startTime = getCurrentTimeMsDouble();  // Get start time

        mouseThreadUse();
        mouseUpdate(mouse);
        mouseThreadRelease();

        // Calculate elapsed time
        endTime = getCurrentTimeMsDouble();  // Get end time
        elapsedTime = endTime - startTime;

        // Calculate the time to sleep
        double sleepTime = targetFrameTime - elapsedTime;

        // If the frame was processed too quickly, sleep to keep the frame rate stable
        if (sleepTime > 0) {
            // Convert sleepTime from milliseconds to microseconds
            // Sleep function may take microseconds
            unsigned int sleepMicro = (unsigned int)(sleepTime * 1000);
            // Sleep
            usleep(sleepMicro);
        }
    }
    return NULL;
}

void mouseInitThread(int historyMaxLength) {
    Display *display = XOpenDisplay(NULL);
    mouse = mouseInit(display, DefaultRootWindow(display), historyMaxLength);
    mouseUpdate(mouse);
    if (pthread_create(&mouseThreadID, NULL, mouseThreadLoop, "mouse-pooling-thread") != 0) {
        fprintf(stderr, "Error [mouseSpawnThread]: failed to create a thread\n");
        return;
    }
}