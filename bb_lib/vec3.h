#pragma once

namespace bb
{
	struct vec3
	{
		vec3();
		vec3(float x, float y, float z);

		void set(float x, float y, float z);

		float dot(const vec3& b) const;

		float length() const;

		void normalize();

		vec3 normalized() const;

		vec3 perpendicular() const;

		vec3 cross(const vec3& c) const;

		vec3 operator*(const float& scalar) const;

		void operator*=(const float& scalar);

		vec3 operator*(const vec3& b) const;

		void operator*=(const vec3& b);

		vec3 operator+(const vec3& b) const;

		void operator+=(const vec3& b);

		vec3 operator-(const vec3& b) const;

		void operator-=(const vec3& b);

		bool operator==(const vec3 &b) const;

		bool operator!=(const vec3 &b) const;

		bool operator<(const vec3& b) const;

		float& operator[](int i);

		float x;
		float y;
		float z;
	};
}