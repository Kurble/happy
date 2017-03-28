#pragma once

namespace bb
{
	struct vec2;

	struct vec3;

	struct vec4;

	// Column-major order
	struct mat4 
	{
	public:
		mat4();
		mat4(const mat4 &other);

		float& operator()(int i, int j);
		float operator()(int i, int j) const;
		mat4& operator= (const mat4 &other);
		mat4& operator= (const float *other);
		bool operator== (const mat4 &other) const;
		bool operator!= (const mat4 &other) const { return !operator==(other); }
		vec4 operator*(const vec4& other) const;
		mat4 operator*(const mat4& other) const;

		void multiply(const mat4& with);

		void scale(vec3 s);
		void translate(vec3 t);
		void rotate(vec4 quat);
		void rotate(float angle, vec3 axis);
		void lookat(vec3 eye, vec3 at, vec3 up);
		void frustum(float left, float right, float bottom, float top, float _near, float _far);
		void perspective(float fovY, float aspect, float _near, float _far);
		void ortho(float left, float right, float bottom, float top, float _near, float _far);
		void swapHandedness();
		void transpose();
		void inverse();
		void identity();
		void interpolate(mat4& other, float x);
		vec3 unproject(vec3 v, float* viewport) const;
		vec2 project(vec3 v, float* viewport) const;
		void setRow(int r, vec4 val);

		float m[16];
	};
}