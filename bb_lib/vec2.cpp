#include "vec2.h"

#include <cmath>
#include <exception>

namespace bb
{
	vec2::vec2() : x(0), y(0) { }
	vec2::vec2(float x, float y) : x(x), y(y)
	{
#ifdef DEBUG
		if (isnan(x)) throw std::exception("x is nan");
		if (isnan(y)) throw std::exception("y is nan");
#endif
	}
	

	void vec2::set(float x, float y)
	{
		this->x = x;
		this->y = y;
#ifdef DEBUG
		if (isnan(x)) throw std::exception("x is nan");
		if (isnan(y)) throw std::exception("y is nan");
#endif
	}

	bool vec2::operator==(const vec2 &b) const 
	{
		return (b.x == x && b.y == y);
	}

	bool vec2::operator!=(const vec2 &b) const {
		return (b.x != x || b.y != y);
	}

	vec2 vec2::operator+(const vec2& b) const
	{
		return vec2(x + b.x, y + b.y);
	}

	vec2 vec2::operator-(const vec2& b) const
	{
		return vec2(x - b.x, y - b.y);
	}

	vec2 vec2::operator*(const vec2& b) const
	{
		return vec2(x * b.x, y * b.y);
	}

	vec2 vec2::operator*(const float& scalar) const
	{
		return vec2(x*scalar, y*scalar);
	}

	bool vec2::operator < (const vec2& other) const 
	{
		if (x == other.x) return y < other.y;
		return x < other.x;
	}

	float vec2::mag() const 
	{
		return sqrtf(x*x + y*y);
	}

	float vec2::mag2() const 
	{
		return x*x + y*y;
	}

	vec2 vec2::normalized() const 
	{
		float f = 1.0f / mag();
		return vec2(
			x * f,
			y * f
		);
	}

	float vec2::dot(const vec2 &o) const 
	{
		return x*o.x + y*o.y;
	}

	float vec2::cross(const vec2 &b) const 
	{
		return x*b.y - y*b.x;
	}
}