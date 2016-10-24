#pragma once

#include "Vector.h"

//--------------------------------------------------------------------------------------------------------------------------
// 2D intersections
//--------------------------------------------------------------------------------------------------------------------------

bool lineLineIntersection(const Vec2 &p1, const Vec2 &p2, const Vec2 &p3, const Vec2 &p4, Vec2 &result, bool inclusive=true);
bool rayRayIntersection(const Vec2 &p1, const Vec2 &p2, const Vec2 &p3, const Vec2 &p4, Vec2 &result);
float pointLineDistanceSquared(const Vec2 &p0, const Vec2 &p1, const Vec2 &point);
Vec2 projectPointOnLine(const Vec2 &p0, const Vec2 &p1, const Vec2 &point);
bool rayCircleIntersection(const Vec2 &p0, const Vec2 &p1, const Vec2 &center, const float &radius, Vec2 &result);
bool lineCircleIntersection(const Vec2 &p0, const Vec2 &p1, const Vec2 &center, const float &radius, Vec2 &result);
bool pointsCollinear(const Vec2 &a, const Vec2 &b, const Vec2 &c);

//--------------------------------------------------------------------------------------------------------------------------
// 3D intersections
//--------------------------------------------------------------------------------------------------------------------------

bool rayCylinderIntersection(const Ray &ray, const Vec3 &position, const float &radius, const float &height);
bool rayAABBIntersection(const Ray &ray, const Vec3 &aa, const Vec3 &bb);