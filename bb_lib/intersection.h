#pragma once

namespace bb
{
	struct vec2;

	struct vec3;

	struct ray;

	//--------------------------------------------------------------------------------------------------------------------------
	// 2D intersections
	//--------------------------------------------------------------------------------------------------------------------------

	bool lineLineIntersection(const vec2 &p1, const vec2 &p2, const vec2 &p3, const vec2 &p4, vec2 &result, bool inclusive = true);
	bool rayRayIntersection(const vec2 &p1, const vec2 &p2, const vec2 &p3, const vec2 &p4, vec2 &result);
	float pointLineDistanceSquared(const vec2 &p0, const vec2 &p1, const vec2 &point);
	vec2 projectPointOnLine(const vec2 &p0, const vec2 &p1, const vec2 &point);
	bool rayCircleIntersection(const vec2 &p0, const vec2 &p1, const vec2 &center, const float &radius, vec2 &result);
	bool lineCircleIntersection(const vec2 &p0, const vec2 &p1, const vec2 &center, const float &radius, vec2 &result);
	bool pointsCollinear(const vec2 &a, const vec2 &b, const vec2 &c);

	//--------------------------------------------------------------------------------------------------------------------------
	// 3D intersections
	//--------------------------------------------------------------------------------------------------------------------------

	bool rayCylinderIntersection(const ray &ray, const vec3 &position, const float &radius, const float &height);
	bool rayAABBIntersection(const ray &ray, const vec3 &aa, const vec3 &bb);
}