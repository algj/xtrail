#pragma once

#include <math.h>
#include <stdlib.h>

#include "./global.c"
#include "./render_area_calc.c"
#include "./dither.c"

#define PIXEL_BATCH 60000
XPoint pixelDrawQueue[PIXEL_BATCH];
int    pixelDrawQueueLen = 0;
void   pixelsFlush(Canvas *canvas) {
    if (pixelDrawQueueLen == 0) return;
    XDrawPoints(canvas->display, canvas->draw, canvas->gc, pixelDrawQueue, pixelDrawQueueLen, CoordModeOrigin);
    pixelDrawQueueLen = 0;
}

void rPixel(Canvas *canvas, Point point, float intensity) {
    if (intensity == 0) return;
    if (point.x == canvas->mouse->state->p.x && point.y == canvas->mouse->state->p.y) return;
    if (intensity >= 1 || canvas->dither == 0 || bayerDitherFloat(point.x, point.y, intensity)) {
        pixelDrawQueue[pixelDrawQueueLen] = (XPoint){point.x, point.y};
        pixelDrawQueueLen++;
        if (pixelDrawQueueLen >= PIXEL_BATCH) {
            pixelsFlush(canvas);
        }
    }
}

void rFCircle(Canvas *c, Point point, float radius, float intensity) {
    if (radius == 0 || intensity == 0) return;
    // calculate the bounding box of the circle
    int x_min = round(point.x - radius);
    int x_max = round(point.x + radius);
    int y_min = round(point.y - radius);
    int y_max = round(point.y + radius);

    // claim render area
    renderAreaClaim(c->area, (Point){x_min, y_min});
    renderAreaClaim(c->area, (Point){x_max, y_max});

    // iterate over each pixel within the bounding box
    for (int x = x_min; x <= x_max; ++x) {
        for (int y = y_min; y <= y_max; ++y) {
            // Check if the pixel is inside the circle
            float distance = hypot(x - point.x, y - point.y);
            if (distance <= radius) {
                // Pixel is inside the circle, draw it with specified intensity
                rPixel(c, (Point){x, y}, intensity);
            }
        }
    }
}

void rRadialGrad(Canvas *c, Point point, float radius, float intensityOut, float intensityIn) {
    if (radius == 0 || (intensityIn == 0 && intensityOut == 0)) return;

    // calculate the bounding box of the circle
    int x_min = round(point.x - radius);
    int x_max = round(point.x + radius);
    int y_min = round(point.y - radius);
    int y_max = round(point.y + radius);

    // claim render area
    renderAreaClaim(c->area, (Point){x_min, y_min});
    renderAreaClaim(c->area, (Point){x_max, y_max});

    // iterate over each pixel within the bounding box
    for (int x = x_min; x <= x_max; ++x) {
        for (int y = y_min; y <= y_max; ++y) {
            // Check if the pixel is inside the circle
            float distance = hypot(x - point.x, y - point.y);
            if (distance <= radius) {
                // Pixel is inside the circle, draw it with specified intensity
                float g = distance / radius;
                rPixel(c, (Point){x, y}, intensityIn * (1 - g) + intensityOut * g);
            }
        }
    }
}

void rLine(Canvas *c, Point point1, Point point2){
    XDrawLine(c->display, c->draw, c->gc, point1.x, point1.y, point2.x, point2.y);

    // claim render area
    renderAreaClaim(c->area, point1);
    renderAreaClaim(c->area, point2);
}

void rTaperedGradLine(Canvas *c, Point point1, float intensity1, float thickness1, Point point2, float intensity2,
                 float thickness2) {
    if (c->dither == 0) {
        intensity1 = intensity2 = 1;
    }
    int dx = abs(point2.x - point1.x);
    int dy = abs(point2.y - point1.y);
    int sx = point1.x < point2.x ? 1 : -1;
    int sy = point1.y < point2.y ? 1 : -1;
    int err = dx - dy;
    int x = point1.x;
    int y = point1.y;
    float line_length = hypot(dx, dy);

    // claim render area
    int maxWidth = (int)(fmax(thickness1, thickness2) + 1);
    renderAreaClaim(c->area, (Point){MIN(point1.x, point2.x) - maxWidth, MIN(point1.y, point2.y) - maxWidth});
    renderAreaClaim(c->area, (Point){MAX(point1.x, point2.x) + maxWidth, MAX(point1.y, point2.y) + maxWidth});

    // Precompute some constant values
    float invLineLength = 1.0f / line_length;
    float invTwo = 1.0f / 2.0f;

    // Calculate the brush pattern pixels
    int maxBrushSize = (int)(fmax(thickness1, thickness2) / 2.0f);
    int brushPixels[2 * maxBrushSize + 1][2 * maxBrushSize + 1];
    for (int i = -maxBrushSize; i <= maxBrushSize; ++i) {
        for (int j = -maxBrushSize; j <= maxBrushSize; ++j) {
            float distance = sqrt(i * i + j * j);
            if (distance <= thickness2 / 2.0) {
                brushPixels[i + maxBrushSize][j + maxBrushSize] = 1;
            } else {
                brushPixels[i + maxBrushSize][j + maxBrushSize] = 0;
            }
        }
    }

    // Draw the line
    while (x != point2.x || y != point2.y) {
        // Calculate the distance along the line from point1 to the current point
        float distance_from_point1 = hypot(x - point1.x, y - point1.y);
        float t = distance_from_point1 * invLineLength;

        // Interpolate intensity and width along the line
        float intensity = intensity1 + t * (intensity2 - intensity1);
        float width = thickness1 + t * (thickness2 - thickness1);

        // Draw pixels around the line based on circular brush pattern
        if (width == 1) {
            rPixel(c, (Point){x, y}, intensity);
        } else {
            for (int i = -maxBrushSize; i <= maxBrushSize; ++i) {
                for (int j = -maxBrushSize; j <= maxBrushSize; ++j) {
                    if (brushPixels[i + maxBrushSize][j + maxBrushSize]) {
                        rPixel(c, (Point){x + i, y + j}, intensity);
                    }
                }
            }
        }

        // Update the Bresenham algorithm for line traversal
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }

    // Draw pixels around the endpoint with intensity2 and thickness2
    if (thickness2 == 1) {
        rPixel(c, point2, intensity2);
    } else {
        for (int i = -maxBrushSize; i <= maxBrushSize; ++i) {
            for (int j = -maxBrushSize; j <= maxBrushSize; ++j) {
                if (brushPixels[i + maxBrushSize][j + maxBrushSize]) {
                    rPixel(c, (Point){point2.x + i, point2.y + j}, intensity2);
                }
            }
        }
    }
}

// void drawAdvLine(Canvas *c, Point point1, float intensity1, float thickness1, Point point2, float intensity2,
//                  float thickness2) {
//     if (c->dither == 0) {
//         intensity1 = intensity2 = 1;
//     }
//     int   dx = abs(point2.x - point1.x);
//     int   dy = abs(point2.y - point1.y);
//     int   sx = point1.x < point2.x ? 1 : -1;
//     int   sy = point1.y < point2.y ? 1 : -1;
//     int   err = dx - dy;
//     int   x = point1.x;
//     int   y = point1.y;
//     float line_length = hypot(dx, dy);

//     // claim render area
//     int maxWidth = MAX(thickness1, thickness2) + 1;
//     renderAreaClaim(c->area, (Point){MIN(point1.x, point2.x) - maxWidth, MIN(point1.y, point2.y) - maxWidth});
//     renderAreaClaim(c->area, (Point){MAX(point1.x, point2.x) + maxWidth, MAX(point1.y, point2.y) + maxWidth});

//     while (x != point2.x || y != point2.y) {
//         // Calculate the distance along the line from point1 to the current point
//         float distance_from_point1 = hypot(x - point1.x, y - point1.y);
//         float t = distance_from_point1 / line_length;

//         // Interpolate intensity and width along the line
//         float intensity = intensity1 + t * (intensity2 - intensity1);
//         float width = thickness1 + t * (thickness2 - thickness1);

//         // Draw pixels around the line based on circular brush pattern
//         if (width == 1) {
//             pixelPut(c, (Point){x, y}, intensity);
//         } else {
//             for (float i = -width / 2.0; i <= width / 2.0; ++i) {
//                 for (float j = -width / 2.0; j <= width / 2.0; ++j) {
//                     // Calculate distance from current point to the line
//                     float distance = sqrt(i * i + j * j);
//                     if (distance <= width / 2.0) {
//                         // Inside the circular brush pattern, draw the pixel
//                         pixelPut(c, (Point){round(x + i), round(y + j)}, intensity);
//                     }
//                 }
//             }
//         }

//         // Update the Bresenham algorithm for line traversal
//         int e2 = 2 * err;
//         if (e2 > -dy) {
//             err -= dy;
//             x += sx;
//         }
//         if (e2 < dx) {
//             err += dx;
//             y += sy;
//         }
//     }

//     // Draw pixels around the endpoint with intensity2 and thickness2
//     if (thickness2 == 1) {
//         pixelPut(c, point2, intensity2);
//     } else {
//         for (float i = -thickness2 / 2.0; i <= thickness2 / 2.0; ++i) {
//             for (float j = -thickness2 / 2.0; j <= thickness2 / 2.0; ++j) {
//                 float distance = sqrt(i * i + j * j);
//                 if (distance <= thickness2 / 2.0) {
//                     pixelPut(c, (Point){round(point2.x + i), round(point2.y + j)}, intensity2);
//                 }
//             }
//         }
//     }
// }