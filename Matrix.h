#pragma once

#include <cmath>
#include <cstring>
#include <iostream>

struct Vec2;
struct Vec3;
struct Vec4;

// Column-major order
class Mat4 {
public:
	Mat4();
	Mat4(const Mat4 &other);

	float& operator()(int i, int j);
	float operator()(int i, int j) const;
	Mat4& operator= (const Mat4 &other);
	Mat4& operator= (const float *other);
	bool operator== (const Mat4 &other);
	bool operator!= (const Mat4 &other) { return !operator==(other); }
	Vec4 operator*(const Vec4& other);
	Mat4 operator*(const Mat4& other);

	void multiply(const Mat4& with);

	void scale(Vec3 s);
	void translate(Vec3 t);
	void rotate(Vec4 quat);
	void rotate(float angle, Vec3 axis);
	void lookat(Vec3 eye, Vec3 at, Vec3 up);
	void frustum(float left, float right, float bottom, float top, float _near, float _far);
	void perspective(float fovY, float aspect, float _near, float _far);
	void ortho(float left, float right, float bottom, float top, float _near, float _far);
	void transpose();
	void inverse();
	void identity();
	void interpolate(Mat4& other, float x);
	Vec3 unproject(Vec3 v, float* viewport);
	Vec2 project(Vec3 v, float* viewport);
	void setRow(int r, Vec4 val);

	float m[16];
};

// Column-major order
class Mat3 {
public:
	Mat3();
	Mat3(const Mat3 &other);
	Mat3(const Mat4 &other);

	float& operator()(int i, int j);
	float operator()(int i, int j) const;
	Mat3& operator= (const Mat3 &other);
	Mat3& operator= (const float *other);
	bool operator== (const Mat3 &other);
	bool operator!= (const Mat3 &other) { return !operator==(other); }
	Vec3 operator*(const Vec3& other);
	Mat3 operator*(const Mat3& other);

	void multiply(const Mat3& with);

	void rotate(Vec4 quat);
	void transpose();
	void adjoint();
	void inverse();
	void identity();
	void setRow(int r, Vec3 row);

	float m[9];
};