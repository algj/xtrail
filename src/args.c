#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int   pos_count;
    int   trail_length;
    int   trail_thickness;
    int   color;
    float refresh_rate;
    float mouse_refresh_rate;
    int   dither;
    char  mouse_separate_thread;
    char  quit;
} ConfigArgs;

void print_help() {
    printf(
        "Usage: xtrail [OPTIONS]\n"
        "Options:\n"
        "  --help                       Display this help message\n"
        "  --pos-count <count>          Set position count to <count>\n"
        "  --trail-length <length>      Set trail length to <length>\n"
        "  --trail-thickness <px>       Set trail thickness to <px>\n"
        "  --color <hex>                Set color to <hex> (e.g. 0x7F7F7F)\n"
        "  --refresh-rate <fps>         Set refresh rate count to <fps>\n"
        "  --mouse-refresh-rate <hz>    Set mouse refresh rate count to <hz> (e.g. 240.00)\n"
        "  --no-dither                  Disable dithering\n"
        "  --mouse-share-thread         Synchronous rendering and mouse pooling\n"
        "Render type options:\n"
        "  --trail                      Render \"trail\" type\n"
        "  --dots                       Render \"dots\" type\n");
}

const char *helpStrs[] = {"--help", "-help", "help", "--h", "-h", "h", "--?", "-?", "?", NULL};

ConfigArgs parseArgs(int argc, char *argv[]) {
    ConfigArgs config;
    config.pos_count = 60;
    config.trail_length = 400;
    config.trail_thickness = 10;
    config.color = 0x7F7F7F;
    config.refresh_rate = -1;
    config.mouse_refresh_rate = -1;
    config.dither = 1;
    config.mouse_separate_thread = 1;
    config.quit = 0;

    for (int i = 1; i < argc; i++) {
        for (int j = 0; helpStrs[j] != NULL; j++) {
            if (strcmp(argv[i], helpStrs[j]) == 0) {
                print_help();
                config.quit = 1;
                return config;
            }
        }
        if (strcmp(argv[i], "--pos-count") == 0) {
            config.pos_count = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--trail-length") == 0) {
            config.trail_length = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--trail-thickness") == 0) {
            config.trail_thickness = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--color") == 0) {
            config.color = strtol(argv[++i], NULL, 0);
        } else if (strcmp(argv[i], "--refresh-rate") == 0) {
            config.refresh_rate = atof(argv[++i]);
        } else if (strcmp(argv[i], "--mouse-refresh-rate") == 0) {
            config.mouse_refresh_rate = atof(argv[++i]);
        } else if (strcmp(argv[i], "--no-dither") == 0) {
            config.dither = 0;
        } else if (strcmp(argv[i], "--mouse-share-thread") == 0) {
            config.mouse_separate_thread = 0;
        }
    }

    if (config.mouse_refresh_rate == -1) {
        // let's keep the mouse refresh rate at least 60...
        if (config.refresh_rate != -1 && config.refresh_rate >= 60) {
            config.mouse_refresh_rate = config.refresh_rate;
        } else {
            config.mouse_refresh_rate = 120;
        }
    }

    return config;
}
