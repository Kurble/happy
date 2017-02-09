#include "intersection.h"
#include "vec2.h"
#include "vec3.h"
#include "ray.h"
#include "math_util.h"
#include <cmath>
#include <algorithm>

namespace bb
{
	using namespace std;

	static bool quadratic(float a, float b, float c, float *t0, float *t1)
	{
		float d = sqrt(b * b - 4 * a * c);

		if (d < 0) return false;

		*t0 = (-b + d) / (2 * a);
		*t1 = (-b - d) / (2 * a);

		return true;
	}

	bool lineLineIntersection(const vec2 &p1, const vec2 &p2, const vec2 &p3, const vec2 &p4, vec2 &result, bool inclusive)
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

	bool rayRayIntersection(const vec2 &p1, const vec2 &p2, const vec2 &p3, const vec2 &p4, vec2 &result)
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

	float pointLineDistanceSquared(const vec2 &p0, const vec2 &p1, const vec2 &point)
	{
		vec2 v = p1 - p0;
		vec2 w = point - p0;

		float c1 = w.dot(v);
		float c2 = v.dot(v);
		float b = fmaxf(0.0f, fminf(1.0f, c1 / c2));

		vec2 pB = p0 + v * b;
		return (point - pB).mag2();
	}

	vec2 projectPointOnLine(const vec2 &p0, const vec2 &p1, const vec2 &point)
	{
		vec2 v = p1 - p0;
		vec2 w = point - p0;

		float c1 = w.dot(v);
		float c2 = v.dot(v);
		float b = fmaxf(0.0f, fminf(1.0f, c1 / c2));

		return p0 + v * b;
	}

	bool rayCircleIntersection(const vec2 &p0, const vec2 &p1, const vec2 &center, const float &radius, vec2 &result)
	{
		vec2 d = p1 - p0;
		vec2 f = p0 - center;

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

	bool lineCircleIntersection(const vec2 &p0, const vec2 &p1, const vec2 &center, const float &radius, vec2 &result)
	{
		vec2 d = p1 - p0;
		vec2 f = p0 - center;

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

	bool rectCircleIntersection(const vec2 &aa, const vec2 &bb, const vec2 &center, const float &radius, vec2 *result)
	{
		vec2 circleDistance = center - ((aa + bb)*0.5f);
		vec2 halfSize = (bb - aa) * 0.5f;
		vec2 sign = vec2(circleDistance.x >= .0f ? 1.f : -1.f, circleDistance.y >= .0f ? 1.f : -1.f);
		circleDistance.x = fabsf(circleDistance.x);
		circleDistance.y = fabsf(circleDistance.y);

		auto calcPenetration = [&]
		{
			if (result)
			{
				vec2 nearestRectPos;
				vec2 nearestCirclePos;

				if ((circleDistance.x <= halfSize.x) && (circleDistance.y <= halfSize.y))
				{
					if (halfSize.y - circleDistance.y < halfSize.x - circleDistance.x)
						nearestRectPos = vec2(circleDistance.x, halfSize.y);
					else 
						nearestRectPos = vec2(halfSize.x, circleDistance.y);

					nearestCirclePos = circleDistance - (nearestRectPos - circleDistance).normalized() * radius;
				}
				else
				{
					nearestRectPos = vec2(fminf(halfSize.x, circleDistance.x), fminf(halfSize.y, circleDistance.y));
					nearestCirclePos = circleDistance + (nearestRectPos - circleDistance).normalized() * radius;
				}

				*result = (nearestRectPos - nearestCirclePos) * sign;
			}
			return true;
		};
		
		if (circleDistance.x >  halfSize.x + radius) return false;
		if (circleDistance.y >  halfSize.y + radius) return false;
		if (circleDistance.x <= halfSize.x) return calcPenetration();
		if (circleDistance.y <= halfSize.y) return calcPenetration();

		float cornerDistanceSquared = (circleDistance - halfSize).dot(circleDistance - halfSize);
		if (cornerDistanceSquared <= (radius * radius))
			return calcPenetration();
		else
			return false;
	}

	bool pointsCollinear(const vec2 &a, const vec2 &b, const vec2 &c)
	{
		return fabsf(
			a.x * (b.y - c.y) +
			b.x * (c.y - a.y) +
			c.x * (a.y - b.y)
		) == 0;
	}

	bool rayCylinderIntersection(const ray &ray, const vec3 &position, const float &radius, const float &height)
	{
		vec2 d = vec2(ray.p0.x, ray.p0.y) - vec2(position.x, position.y);
		vec2 D = vec2(ray.p1.x - ray.p0.x, ray.p1.y - ray.p0.y);

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

	bool rayAABBIntersection(const ray &ray, const vec3 &aa, const vec3 &bb, float *result)
	{
		vec3 dir = ray.p1 - ray.p0;
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

		if (result)
		{
			*result = tmax;
			if (tmin > 0) *result = tmin;
		}

		return true;
	}
}