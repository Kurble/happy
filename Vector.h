#pragma once

#include <cmath>

#include "Matrix.h"

static const float epsilon = 0.00001f;

template <class T> T lerp(T a, T b, float x)
{
	return a*x + b*(1-x);
}

struct Vec2
{
	Vec2() {}
	Vec2(float x, float y) : x(x), y(y) {}

	void set(float x, float y)
	{
		this->x = x;
		this->y = y;
	}

	bool operator==(const Vec2 &b) const {
		return (b.x == x && b.y == y);
	}

	Vec2 operator+(const Vec2& b) const
	{
		return{ x + b.x, y + b.y };
	}

	Vec2 operator-(const Vec2& b) const
	{
		return{ x - b.x, y - b.y };
	}

	Vec2 operator*(const Vec2& b) const
	{
		return{ x * b.x, y * b.y };
	}

	Vec2 operator*(const float& scalar) const
	{
		return{ x*scalar, y*scalar };
	}

	bool operator < (const Vec2& other) const {
		if (x == other.x) return y < other.y;
		return x < other.x;
	}

	float mag() {
		return sqrtf(x*x + y*y);
	}

	Vec2 normalized() {
		float f = 1.0f / mag();
		return Vec2(
			x * f,
			y * f
			);
	}

	float dot(Vec2 &o) {
		return x*o.x + y*o.y;
	}

	float x;
	float y;
};

struct Vec3
{
	Vec3() :x(0), y(0), z(0) {}
	Vec3(float x, float y, float z) : x(x), y(y), z(z) {}

	void set(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	float dot(const Vec3& b) const
	{
		return x*b.x + y*b.y + z*b.z;
	}

	float length() const
	{
		return sqrtf(x*x + y*y + z*z);
	}

	void normalize() {
		float length = sqrt(x*x + y*y + z*z);

		if (length) {
			length = 1.0f / length;
			x *= length;
			y *= length;
			z *= length;
		}
	}

	Vec3 normalized() const
	{
		float length = sqrt(x*x + y*y + z*z);
		if (length) length = 1.0f / length;

		Vec3 r = { x*length, y*length, z*length };
		return r;
	}

	Vec3 cross(const Vec3& c) const
	{
		Vec3 a;
		a.x = (y*c.z) - (z*c.y);
		a.y = (z*c.x) - (x*c.z);
		a.z = (x*c.y) - (y*c.x);

		return a;
	}

	Vec3 operator*(const float& scalar) const
	{
		return{ x*scalar, y*scalar, z*scalar };
	}

	void operator*=(const float& scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
	}

	Vec3 operator*(const Vec3& b) const
	{
		return{ x*b.x, y*b.y, z*b.z };
	}

	void operator*=(const Vec3& b)
	{
		x *= b.x;
		y *= b.y;
		z *= b.z;
	}

	Vec3 operator+(const Vec3& b) const
	{
		return{ x + b.x, y + b.y, z + b.z };
	}

	void operator+=(const Vec3& b)
	{
		x += b.x;
		y += b.y;
		z += b.z;
	}

	Vec3 operator-(const Vec3& b) const
	{
		return{ x - b.x, y - b.y, z - b.z };
	}

	void operator-=(const Vec3& b)
	{
		x -= b.x;
		y -= b.y;
		z -= b.z;
	}

	bool operator==(const Vec3 &b) const
	{
		return (b.x == x && b.y == y && b.z == z);
	}

	bool operator!=(const Vec3 &b) const { return !operator==(b); }

	bool operator<(const Vec3& b) const
	{
		if (x - b.x < -0.05f) return true;
		if (x - b.x <  0.05f)
		{
			if (y - b.y < -0.05f) return true;
			if (y - b.y <  0.05f)
			{
				return z - b.z < -0.05f;
			}
		}

		return false;
	}

	float& operator[](int i) {
		return (&x)[i];
	}

	float x;
	float y;
	float z;
};

struct Vec4
{
	Vec4() {}
	Vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

	void set(const float x, const float y, const float z, const float w)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	void setEuler(const float yaw, const float pitch, const float roll)
	{
		float c1 = cosf(yaw   * 0.5f);
		float c2 = cosf(roll  * 0.5f);
		float c3 = cosf(pitch * 0.5f);
		float s1 = sinf(yaw   * 0.5f);
		float s2 = sinf(roll  * 0.5f);
		float s3 = sinf(pitch * 0.5f);

		w = c1*c2*c3 - s1*s2*s3;
		x = s1*s2*c3 + c1*c2*s3;
		y = s1*c2*c3 + c1*s2*s3;
		z = c1*s2*c3 - s1*c2*s3;
	}

	float dot(const Vec4& b) const
	{
		return x*b.x + y*b.y + z*b.z + w*b.w;
	}

	Vec4 normalized() const
	{
		float length = 1.0f / sqrtf(x*x + y*y + z*z + w*w);
		Vec4 r = { x*length, y*length, z*length, w*length };
		return r;
	}

	Vec4 slerp(const float& x, const Vec4& other) const
	{
		if (x <= 0.0f)
			return *this;
		if (x >= 1.0f)
			return other;

		Vec4 self = *this;
		Vec4 result = other;
		float c = dot(other);

		if (c < 0)
		{
			result = result * -1;
			c = -c;
		}

		if (c > 1.0f - epsilon)
			return lerp(self, result, x).normalized();

		float a = acosf(c);
		return (self * sinf((1.0f - x) * a) + result * sinf(x * a)) * (1.0f / sinf(a));
	}

	static Vec4 slerp(const float& x, const Vec4& a, const Vec4& b)
	{
		return a.slerp(x, b);
	}

	Vec4 multiplyQuaternion(const Vec4& other) const
	{
		Vec4 res;
		const Vec4& q = *this;
		const Vec4& p = other;

		res.w = q.w * p.w - q.x * p.x - q.y * p.y - q.z * p.z;
		res.x = q.w * p.x + q.x * p.w + q.y * p.z - q.z * p.y;
		res.y = q.w * p.y + q.y * p.w + q.z * p.x - q.x * p.z;
		res.z = q.w * p.z + q.z * p.w + q.x * p.y - q.y * p.x;

		return res;
	}

	static Vec4 multiplyQuaternion(const Vec4& a, const Vec4& b)
	{
		return a.multiplyQuaternion(b);
	}

	static Vec4 rotationBetween(const Vec3& a, const Vec3& b)
	{
		Vec3 axis = a.cross(b).normalized();
		float angle = acosf(a.dot(b));

		return Vec4(axis.x * sinf(angle*0.5f),
			axis.y * sinf(angle*0.5f),
			axis.z * sinf(angle*0.5f),
			cosf(angle*0.5f));
	}

	static Vec4 rotationBetween(const Vec3& a, const Vec3& b, const Vec3& axis, const bool snap = false)
	{
		float angle = acosf(a.dot(b));

		if (snap) {
			angle = 0.261799388f * floorf((angle / 0.261799388f) + 0.5f);
		}

		return Vec4(axis.x * sinf(angle*0.5f),
			axis.y * sinf(angle*0.5f),
			axis.z * sinf(angle*0.5f),
			cosf(angle*0.5f));
	}

	static Vec4 lookAt(const Vec3& source, const Vec3& dest)
	{
		Vec3 forward = (dest - source).normalized();

		float dot = Vec3(0, 0, -1).dot(forward);

		if (fabsf(dot - (-1.0f)) < 0.000001f)
		{
			return Vec4(0, 1, 0, 3.141592f);
		}
		if (fabsf(dot - (1.0f)) < 0.000001f)
		{
			return Vec4(0, 0, 0, 1);
		}

		float rotAngle = cosf(dot);
		Vec3 axis = Vec3(0, 0, -1).cross(forward).normalized();

		return Vec4(
			axis.x * sinf(rotAngle*0.5f),
			axis.y * sinf(rotAngle*0.5f),
			axis.z * sinf(rotAngle*0.5f),
			cosf(rotAngle*0.5f));
	}

	bool operator==(const Vec4 &b) const
	{
		return (b.x == x && b.y == y && b.z == z && b.w == w);
	}

	Vec3 operator*(const Vec3& b)
	{
		Mat3 rotation;
		rotation.identity();
		rotation.rotate(*this);

		return rotation * b;
	}

	Vec4 operator*(const float& scalar)
	{
		return{ x*scalar, y*scalar, z*scalar, w*scalar };
	}

	Vec4 operator+(const Vec4& b)
	{
		return{ x + b.x, y + b.y, z + b.z, w + b.w };
	}

	float& operator[](int i) {
		return (&x)[i];
	}

	float x;
	float y;
	float z;
	float w;
};

struct Ray
{
	Vec3 p0, p1;

	Vec3 t(float _t) {
		return p0 + (p1 - p0)*_t;
	}

	float distanceToPoint(Vec3 p2, float* _t = 0)
	{
		Vec3 v = p1 - p0;
		Vec3 w = p2 - p0;

		float c1 = w.dot(v);
		float c2 = v.dot(v);
		float b = c1 / c2;

		if (_t) *_t = b;

		Vec3 Pb = p0 + v * b;
		return (p2 - Pb).length();
	}

	float distanceToRay(Ray r2, float* _t = 0)
	{//http://geomalgorithms.com/a07-_distance.html
		Ray& r1 = *this;

		Vec3 u = r1.p1 - r1.p0;
		Vec3 v = r2.p1 - r2.p0;
		Vec3 w = r1.p0 - r2.p0;

		float a = u.dot(u);
		float b = u.dot(v);
		float c = v.dot(v);
		float d = u.dot(w);
		float e = v.dot(w);
		float D = a*c - b*b;
		float sc, tc;

		if (D < 0.0001f) {
			sc = 0;
			tc = (b>c ? d / b : e / c);
		}
		else {
			sc = (b*e - c*d) / D;
			tc = (a*e - b*d) / D;
		}

		if (_t) *_t = tc;

		Vec3 dP = w + (u * sc) - (v * tc);
		return dP.length();
	}
};

struct Plane
{
	Vec3 p0;
	Vec3 n;

	bool intersectRay(Ray r, float* _t = 0)
	{
		Vec3 l0 = r.p0;
		Vec3 l = r.p1 - r.p0;

		// assuming vectors are all normalized
		float denom = n.dot(l);
		if (denom != 0) {
			Vec3 p0l0 = p0 - l0;
			float t = p0l0.dot(n) / denom;

			if (_t) *_t = t;

			return true;
		}

		return false;
	}
};