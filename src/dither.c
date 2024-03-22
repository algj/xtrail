#pragma once

// Bayer matrix for dithering
short bayerMatrix[4][4] = {
    {0, 8, 2, 10},
    {12, 4, 14, 6},
    {3, 11, 1, 9},
    {15, 7, 13, 5},
};
float bayerMatrixFloat[4][4] = {
    {0.0f, 0.5f, 0.125f, 0.625f},
    {0.75f, 0.25f, 0.875f, 0.375f},
    {0.1875f, 0.6875f, 0.0625f, 0.5625f},
    {0.9375f, 0.4375f, 0.8125f, 0.3125f}
};

char inline bayerDitherFloat(short x, short y, float val) { return bayerMatrixFloat[x % 4][y % 4] < val; }
char inline bayerDither16(short x, short y, char val) { return bayerMatrix[x % 4][y % 4] < val; }