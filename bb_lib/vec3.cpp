#include "vec3.h"
#include <cmath>

namespace bb
{
	vec3::vec3() :x(0), y(0), z(0) {}
	vec3::vec3(float x, float y, float z) : x(x), y(y), z(z) {}

	void vec3::set(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	float vec3::dot(const vec3& b) const
	{
		return x*b.x + y*b.y + z*b.z;
	}

	float vec3::length() const
	{
		return sqrtf(x*x + y*y + z*z);
	}

	void vec3::normalize() {
		float length = sqrt(x*x + y*y + z*z);

		if (length) {
			length = 1.0f / length;
			x *= length;
			y *= length;
			z *= length;
		}
	}

	vec3 vec3::normalized() const
	{
		float length = sqrt(x*x + y*y + z*z);
		if (length) length = 1.0f / length;

		vec3 r = { x*length, y*length, z*length };
		return r;
	}

	vec3 vec3::perpendicular() const
	{
		if (x <= y && x <= z) return cross({ 1, 0, 0 });
		if (y <= x && y <= z) return cross({ 0, 1, 0 });
		else                  return cross({ 0, 0, 1 });
	}

	vec3 vec3::cross(const vec3& c) const
	{
		vec3 a;
		a.x = (y*c.z) - (z*c.y);
		a.y = (z*c.x) - (x*c.z);
		a.z = (x*c.y) - (y*c.x);

		return a;
	}

	vec3 vec3::operator*(const float& scalar) const
	{
		return{ x*scalar, y*scalar, z*scalar };
	}

	void vec3::operator*=(const float& scalar)
	{
		x *= scalar;
		y *= scalar;
		z *= scalar;
	}

	vec3 vec3::operator*(const vec3& b) const
	{
		return{ x*b.x, y*b.y, z*b.z };
	}

	void vec3::operator*=(const vec3& b)
	{
		x *= b.x;
		y *= b.y;
		z *= b.z;
	}

	vec3 vec3::operator+(const vec3& b) const
	{
		return{ x + b.x, y + b.y, z + b.z };
	}

	void vec3::operator+=(const vec3& b)
	{
		x += b.x;
		y += b.y;
		z += b.z;
	}

	vec3 vec3::operator-(const vec3& b) const
	{
		return{ x - b.x, y - b.y, z - b.z };
	}

	void vec3::operator-=(const vec3& b)
	{
		x -= b.x;
		y -= b.y;
		z -= b.z;
	}

	bool vec3::operator==(const vec3 &b) const
	{
		return (b.x == x && b.y == y && b.z == z);
	}

	bool vec3::operator!=(const vec3 &b) const { return !operator==(b); }

	bool vec3::operator<(const vec3& b) const
	{
		if (x - b.x < -0.05f) return true;
		if (x - b.x < 0.05f)
		{
			if (y - b.y < -0.05f) return true;
			if (y - b.y < 0.05f)
			{
				return z - b.z < -0.05f;
			}
		}

		return false;
	}

	float& vec3::operator[](int i) 
	{
		return (&x)[i];
	}
}