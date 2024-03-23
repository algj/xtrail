#pragma once

#include <math.h>

typedef struct {
    int x;
    int y;
} Point;
typedef struct {
    float x;
    float y;
} FPoint;

// Distance between two points, obviously
float pointGetDistance(Point p1, Point p2) {
    if (p1.x == p2.x && p1.y == p2.y) return 0;
    return hypot(p2.x - p1.x, p2.y - p1.y);
}

// Function to interpolate between two points based on a given percentage
Point pointInterpolate(Point p1, Point p2, float percentage) {
    return (Point){
        p1.x * (1 - percentage) + p2.x * percentage,
        p1.y * (1 - percentage) + p2.y * percentage,
    };
}