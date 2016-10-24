#include "stdafx.h"
#include "Intersection.h"

static bool quadratic(float a, float b, float c, float *t0, float *t1)
{
	float d = sqrt(b * b - 4 * a * c);

	if (d < 0) return false;

	*t0 = (-b + d) / (2 * a);
	*t1 = (-b - d) / (2 * a);

	return true;
}

bool lineLineIntersection(const Vec2 &p1, const Vec2 &p2, const Vec2 &p3, const Vec2 &p4, Vec2 &result, bool inclusive)
{
	float rxs = (p2.x - p1.x)*(p4.y - p3.y) - (p2.y - p1.y)*(p4.x - p3.x);
	if (rxs != 0)
	{
		float t = ((p3.x - p1.x)*(p2.y - p1.y) - (p3.y - p1.y)*(p2.x - p1.x)) / rxs;
		float u = ((p3.x - p1.x)*(p4.y - p3.y) - (p3.y - p1.y)*(p4.x - p3.x)) / rxs;

		if (inclusive)
		{
			if (t >= 0 && t <= 1 &&
				u >= 0 && u <= 1)
			{
				result = p1 + (p2 - p1) * u;
				return true;
			}
		}
		else
		{
			if (t > 0 && t < 1 &&
				u > 0 && u < 1)
			{
				result = p1 + (p2 - p1) * u;
				return true;
			}
		}
	}

	return false;
}

bool rayRayIntersection(const Vec2 &p1, const Vec2 &p2, const Vec2 &p3, const Vec2 &p4, Vec2 &result)
{
	float rxs = (p2.x - p1.x)*(p4.y - p3.y) - (p2.y - p1.y)*(p4.x - p3.x);
	if (rxs != 0)
	{
		float u = ((p3.x - p1.x)*(p4.y - p3.y) - (p3.y - p1.y)*(p4.x - p3.x)) / rxs;
		result = p1 + (p2 - p1) * u;
		return true;
	}

	return false;
}

float pointLineDistanceSquared(const Vec2 &p0, const Vec2 &p1, const Vec2 &point)
{
	Vec2 v = p1 - p0;
	Vec2 w = point - p0;

	float c1 = w.dot(v);
	float c2 = v.dot(v);
	float b = fmaxf(0.0f, fminf(1.0f, c1 / c2));

	Vec2 pB = p0 + v * b;
	return (point - pB).mag2();
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

bool rayCircleIntersection(const Vec2 &p0, const Vec2 &p1, const Vec2 &center, const float &radius, Vec2 &result)
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

	if (t1 >= 0.0f)
	{
		result = lerp(p0, p1, t1);
		return true;
	}

	if (t2 >= 0.0f)
	{
		result = lerp(p0, p1, t2);
		return true;
	}

	return false;
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

bool pointsCollinear(const Vec2 &a, const Vec2 &b, const Vec2 &c)
{
	return fabsf(
		 a.x * (b.y - c.y) +
		 b.x * (c.y - a.y) +
		 c.x * (a.y - b.y)
		) == 0;
}

bool rayCylinderIntersection(const Ray &ray, const Vec3 &position, const float &radius, const float &height)
{
	Vec2 d = Vec2(ray.p0.x, ray.p0.y) - Vec2(position.x, position.y);
	Vec2 D = Vec2(ray.p1.x - ray.p0.x, ray.p1.y - ray.p0.y);

	float a = D.dot(D);
	float b = d.dot(D);
	float c = d.dot(d) - radius * radius;

	float disc = b * b - a * c;

	if (disc < 0.0f)
	{
		return false;
	}

	float sqrtDisc = sqrtf(disc);
	float invA = 1.0f / a;

	float t[2];
	t[0] = (-b - sqrtDisc) * invA;
	t[1] = (-b + sqrtDisc) * invA;

	float h[2] = { lerp(ray.p0, ray.p1, t[0]).z, lerp(ray.p0, ray.p1, t[1]).z };

	if (h[0] >= position.z && h[0] <= position.z + height) return true;
	if (h[1] >= position.z && h[1] <= position.z + height) return true;
	if (h[0] > position.z + height && h[1] < position.z) return true;
	if (h[1] > position.z + height && h[0] < position.z) return true;

	return false;
}

bool rayAABBIntersection(const Ray &ray, const Vec3 &aa, const Vec3 &bb)
{
	Vec3 dir = ray.p1 - ray.p0;
	float tmin = (aa.x - ray.p0.x) / dir.x;
	float tmax = (bb.x - ray.p0.x) / dir.x;

	if (tmin > tmax) swap(tmin, tmax);

	float tymin = (aa.y - ray.p0.y) / dir.y;
	float tymax = (bb.y - ray.p0.y) / dir.y;

	if (tymin > tymax) swap(tymin, tymax);

	if ((tmin > tymax) || (tymin > tmax))
		return false;

	if (tymin > tmin)
		tmin = tymin;

	if (tymax < tmax)
		tmax = tymax;

	float tzmin = (aa.z - ray.p0.z) / dir.z;
	float tzmax = (bb.z - ray.p0.z) / dir.z;

	if (tzmin > tzmax) swap(tzmin, tzmax);

	if ((tmin > tzmax) || (tzmin > tmax))
		return false;

	if (tzmin > tmin)
		tmin = tzmin;

	if (tzmax < tmax)
		tmax = tzmax;

	return true;
}