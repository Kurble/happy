#include "stdafx.h"
#include "Intersection.h"

bool lineLineIntersection(const Vec2 &p1, const Vec2 &p2, const Vec2 &p3, const Vec2 &p4, Vec2 &result)
{
	float d = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
	if (d == 0) return false; // no intersection

	float pre  = (p1.x*p2.y - p1.y*p2.x);
	float post = (p3.x*p4.y - p3.y*p4.x);

	float x = (pre * (p3.x - p4.x) - (p1.x - p2.x) * post) / d;
	if (x < fminf(p1.x, p2.x) || 
		x > fmaxf(p1.x, p2.x) || 
		x < fminf(p3.x, p4.x) || 
		x > fmaxf(p3.x, p4.x)) return false;

	float y = (pre * (p3.y - p4.y) - (p1.y - p2.y) * post) / d;
	if (y < fminf(p1.y, p2.y) || 
		y > fmaxf(p1.y, p2.y) || 
		y < fminf(p3.y, p4.y) || 
		y > fmaxf(p3.y, p4.y)) return false;

	result.set(x, y);
	return true;
}

float pointLineDistanceSquared(const Vec2 &p0, const Vec2 &p1, const Vec2 &point)
{
	Vec2 v = p1 - p0;
	Vec2 w = point - p0;

	float c1 = w.dot(v);
	float c2 = v.dot(v);
	float b = fmaxf(0.0f, fminf(1.0f, c1 / c2));

	Vec2 pB = p0 + v * b;
	return (point - pB).dot(point - pB);
}

Vec2 projectPointOnLine(const Vec2 &p0, const Vec2 &p1, const Vec2 &point)
{
	Vec2 v = p1 - p0;
	Vec2 w = point - p0;

	float c1 = w.dot(v);
	float c2 = v.dot(v);
	float b = fmaxf(0.0f, fminf(1.0f, c1 / c2));

	return p0 + v * b;
}

bool lineCircleIntersection(const Vec2 &p0, const Vec2 &p1, const Vec2 &center, const float &radius, Vec2 &result)
{
	Vec2 d = p1 - p0;
	Vec2 f = p0 - center;

	float a = d.dot(d);
	float b = 2.0f * f.dot(d);
	float c = f.dot(f) - radius*radius;

	float disc = b*b - 4.0f*a*c;
	if (disc < 0) return false;

	disc = sqrtf(disc);
	float t1 = (-b - disc) / (2.0f * a);
	float t2 = (-b + disc) / (2.0f * a);

	if (t1 >= 0.0f && t1 <= 1.0f)
	{
		result = lerp(p0, p1, t1);
		return true;
	}

	if (t2 >= 0.0f && t2 <= 1.0f)
	{
		result = lerp(p0, p1, t2);
		return true;
	}

	return false;
}