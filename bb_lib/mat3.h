#pragma once

namespace bb
{
	struct vec3;

	struct vec4;

	struct mat4;

	// Column-major order
	struct mat3 
	{
	public:
		mat3();
		mat3(const mat3 &other);
		mat3(const mat4 &other);

		float& operator()(int i, int j);
		float operator()(int i, int j) const;
		mat3& operator= (const mat3 &other);
		mat3& operator= (const float *other);
		bool operator== (const mat3 &other);
		bool operator!= (const mat3 &other) { return !operator==(other); }
		vec3 operator*(const vec3& other);
		mat3 operator*(const mat3& other);

		void multiply(const mat3& with);

		void rotate(vec4 quat);
		void transpose();
		void adjoint();
		void inverse();
		void identity();
		void setRow(int r, vec3 row);

		float m[9];
	};
}