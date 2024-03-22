#pragma once

// Bayer matrix for dithering
short bayerMatrix[4][4] = {
    {0, 8, 2, 10},
    {12, 4, 14, 6},
    {3, 11, 1, 9},
    {15, 7, 13, 5},
};
float bayerMatrixFloat[4][4] = {
    {0.00000000000000000, 0.53333333333333333, 0.13333333333333333, 0.66666666666666667},
    {0.80000000000000000, 0.26666666666666666, 0.93333333333333333, 0.40000000000000000},
    {0.20000000000000000, 0.73333333333333333, 0.06666666666666667, 0.60000000000000000},
    {1.00000000000000000, 0.46666666666666667, 0.86666666666666667, 0.33333333333333333},
};

char bayerDitherFloat(short x, short y, float val) { return bayerMatrixFloat[x % 4][y % 4] < val; }
char bayerDither16(short x, short y, char val) { return bayerMatrix[x % 4][y % 4] < val; }