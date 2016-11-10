#pragma once

namespace bb
{
	struct vec2
	{
		vec2();
		vec2(float x, float y);

		void set(float x, float y);

		bool operator==(const vec2 &b) const;

		bool operator!=(const vec2 &b) const;

		vec2 operator+(const vec2& b) const;

		vec2 operator-(const vec2& b) const;

		vec2 operator*(const vec2& b) const;

		vec2 operator*(const float& scalar) const;

		bool operator < (const vec2& other) const;

		float mag() const;

		float mag2() const;

		vec2 normalized() const;

		float dot(const vec2 &o) const;

		float cross(const vec2 &b) const;

		float x;
		float y;
	};
}