#pragma once

#include <math.h>
#include <stdlib.h>

#include "./dither.c"
#include "./global.c"
#include "./render_area_calc.c"

// note, when you're setting pixel colors, you're always adding to existing pixel values rather than replacing them,
// unless the value is -1...
// colors:
//     -1 = clear pixel (reset to 0)
//      0 = no change (even if the pixel is currently 1)
// 0 to 1 = dithering (additive)
//      1 = set pixel to 1

void inline rPixelRaw(XImage *image, int x, int y, char color) {
    unsigned char *pixelByte = (unsigned char *)image->data + (y)*image->bytes_per_line + (x) / 8;
    unsigned char  pixelMask = 1 << ((x) % 8);
    if (color)
        *pixelByte |= pixelMask;
    else
        *pixelByte &= ~pixelMask;
}

void inline rPixelAdd(Canvas *canvas, Point point, float color) {
    if (point.x >= canvas->image->width || point.y >= canvas->image->height || point.x < 0 || point.y < 0) return;
    if (color == -1) {
        rPixelRaw(canvas->image, point.x, point.y, 0);
        return;
    }
    if (color >= 1 || canvas->dither == 0 || bayerDitherFloat(point.x, point.y, color))
        rPixelRaw(canvas->image, point.x, point.y, 1);
}

void rFCircle(Canvas *c, Point point, float radius, float color) {
    if (radius == 0 || color == 0) return;
    // calculate the bounding box of the circle
    int x_min = round(point.x - radius);
    int x_max = round(point.x + radius) + 1;
    int y_min = round(point.y - radius);
    int y_max = round(point.y + radius) + 1;

    // claim render area
    if (color != -1) {
        renderAreaClaim(c->area, (Point){x_min, y_min});
        renderAreaClaim(c->area, (Point){x_max, y_max});
    }

    // iterate over each pixel within the bounding box
    for (int x = x_min; x <= x_max; ++x) {
        for (int y = y_min; y <= y_max; ++y) {
            // Check if the pixel is inside the circle
            float distance = hypot(x - point.x, y - point.y);
            if (distance <= radius) {
                // Pixel is inside the circle, draw it with specified intensity
                rPixelAdd(c, (Point){x, y}, color);
            }
        }
    }
}

void rRadialGrad(Canvas *c, Point point, float radius, float intensityOut, float intensityIn) {
    if (radius == 0 || (intensityIn == 0 && intensityOut == 0)) return;

    // calculate the bounding box of the circle
    int x_min = round(point.x - radius);
    int x_max = round(point.x + radius) + 1;
    int y_min = round(point.y - radius);
    int y_max = round(point.y + radius) + 1;

    // claim render area
    if (intensityIn != -1 && intensityIn != -1) {
        renderAreaClaim(c->area, (Point){x_min, y_min});
        renderAreaClaim(c->area, (Point){x_max + 1, y_max + 1});
    }

    // iterate over each pixel within the bounding box
    for (int x = x_min; x <= x_max; ++x) {
        for (int y = y_min; y <= y_max; ++y) {
            // Check if the pixel is inside the circle
            float distance = hypot(x - point.x, y - point.y);
            if (distance <= radius) {
                // Pixel is inside the circle, draw it with specified intensity
                float g = distance / radius;
                rPixelAdd(c, (Point){x, y}, intensityIn * (1 - g) + intensityOut * g);
            }
        }
    }
}

void rLine(Canvas *c, Point point1, Point point2, char color) {
    int x0 = point1.x;
    int y0 = point1.y;
    int x1 = point2.x;
    int y1 = point2.y;

    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = x0 < x1 ? 1 : -1;
    int sy = y0 < y1 ? 1 : -1;
    int err = dx - dy;

    while (x0 != x1 || y0 != y1) {
        rPixelAdd(c, (Point){x0, y0}, 1);
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y0 += sy;
        }
    }
    rPixelAdd(c, (Point){x1, y1}, 1);

    // claim render area
    if (color != -1) {
        renderAreaClaim(c->area, point1);
        renderAreaClaim(c->area, point2);
    }
}

void rTaperedGradLine(Canvas *c, Point point1, float color1, float thickness1, Point point2, float color2, float thickness2) {
    if (c->dither == 0) color1 = color2 = 1;

    int   dx = abs(point2.x - point1.x);
    int   dy = abs(point2.y - point1.y);
    int   sx = point1.x < point2.x ? 1 : -1;
    int   sy = point1.y < point2.y ? 1 : -1;
    int   err = dx - dy;
    int   x = point1.x;
    int   y = point1.y;
    float line_length = hypot(dx, dy);

    // claim render area
    if (color1 != -1 && color2 != -1) {
        int maxWidth = (int)(fmax(thickness1, thickness2) + 1);
        renderAreaClaim(c->area, (Point){MIN(point1.x, point2.x) - maxWidth, MIN(point1.y, point2.y) - maxWidth});
        renderAreaClaim(c->area, (Point){MAX(point1.x, point2.x) + maxWidth, MAX(point1.y, point2.y) + maxWidth});
    }

    // Precompute some constant values
    float invLineLength = 1.0f / line_length;
    float invTwo = 1.0f / 2.0f;
    int   maxBrushSize = (int)(fmax(thickness1, thickness2) / 2.0f);

    // Precompute brush pattern pixels
    int brushPixels[2 * maxBrushSize + 1][2 * maxBrushSize + 1];
    for (int i = -maxBrushSize; i <= maxBrushSize; ++i) {
        for (int j = -maxBrushSize; j <= maxBrushSize; ++j) {
            float distance = sqrt(i * i + j * j);
            brushPixels[i + maxBrushSize][j + maxBrushSize] = distance <= thickness2 / 2.0;
        }
    }

    // Draw the line
    while (x != point2.x || y != point2.y) {
        // Calculate the distance along the line from point1 to the current point
        float distance_from_point1 = hypot(x - point1.x, y - point1.y);
        float t = distance_from_point1 * invLineLength;

        // Interpolate intensity and width along the line
        float intensity = color1 + t * (color2 - color1);
        float width = thickness1 + t * (thickness2 - thickness1);

        // Draw pixels around the line based on circular brush pattern
        if (width == 1) {
            rPixelAdd(c, (Point){x, y}, intensity);
        } else {
            for (int i = -maxBrushSize; i <= maxBrushSize; ++i) {
                for (int j = -maxBrushSize; j <= maxBrushSize; ++j) {
                    if (brushPixels[i + maxBrushSize][j + maxBrushSize]) {
                        rPixelAdd(c, (Point){x + i, y + j}, intensity);
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
        rPixelAdd(c, point2, color2);
    } else {
        for (int i = -maxBrushSize; i <= maxBrushSize; ++i) {
            for (int j = -maxBrushSize; j <= maxBrushSize; ++j) {
                if (brushPixels[i + maxBrushSize][j + maxBrushSize]) {
                    rPixelAdd(c, (Point){point2.x + i, point2.y + j}, color2);
                }
            }
        }
    }
}