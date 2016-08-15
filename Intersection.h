#pragma once

#include "Vector.h"

bool lineLineIntersection(const Vec2 &p1, const Vec2 &p2, const Vec2 &p3, const Vec2 &p4, Vec2 &result);
float pointLineDistanceSquared(const Vec2 &p0, const Vec2 &p1, const Vec2 &point);
Vec2 projectPointOnLine(const Vec2 &p0, const Vec2 &p1, const Vec2 &point);
bool lineCircleIntersection(const Vec2 &p0, const Vec2 &p1, const Vec2 &center, const float &radius, Vec2 &result);