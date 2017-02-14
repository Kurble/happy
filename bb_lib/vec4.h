#pragma once

namespace bb
{
	struct vec3;

	struct vec4
	{
		vec4();
		vec4(float x, float y, float z, float w);
		vec4(const vec3 &pos);

		void set(const float x, const float y, const float z, const float w);

		void setEuler(const float yaw, const float pitch, const float roll);

		float dot(const vec4& b) const;

		vec4 normalized() const;

		vec4 slerp(const float& x, const vec4& other) const;

		static vec4 slerp(const float& x, const vec4& a, const vec4& b);

		vec4 multiplyQuaternion(const vec4& other) const;

		static vec4 multiplyQuaternion(const vec4& a, const vec4& b);

		static vec4 rotationBetween(const vec3& a, const vec3& b);

		static vec4 rotationBetween(const vec3& a, const vec3& b, const vec3& axis, const bool snap = false);

		static vec4 lookAt(const vec3& source, const vec3& dest);
		
		static vec4 quatSnap(const vec4 &quat, const float angle);

		bool operator==(const vec4 &b) const;

		vec3 operator*(const vec3& b);

		vec4 operator*(const float& scalar) const;

		vec4 operator+(const vec4& b) const;

		vec4 operator-(const vec4& b) const;

		float& operator[](int i);

		float x;
		float y;
		float z;
		float w;
	};
}