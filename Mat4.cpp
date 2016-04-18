#include "stdafx.h"
#include "Matrix.h"
#include "Vector.h"
#define PI 3.1415926535897932384626433832795f

Mat4::Mat4() {
	for (int i = 0; i < 16; i++) {
		m[i] = 0;
	}
}

Mat4::Mat4(const Mat4& other) {
	for (int i = 0; i < 16; i++) {
		m[i] = other.m[i];
	}
}

float& Mat4::operator()(int i, int j) {
	return m[i * 4 + j];
}

float Mat4::operator()(int i, int j) const {
	return m[i * 4 + j];
}

Mat4& Mat4::operator= (const Mat4 &other) {
	for (int i = 0; i < 16; i++) {
		m[i] = other.m[i];
	}

	return *this;
}

Mat4& Mat4::operator= (const float *other) {
	for (int i = 0; i < 16; i++) {
		m[i] = other[i];
	}

	return *this;
}

bool Mat4::operator== (const Mat4 &other) {
	for (int i = 0; i < 16; i++) {
		if (m[i] != other.m[i]) {
			return false;
		}
	}

	return true;
}

Vec4 Mat4::operator* (const Vec4 &other) {
	Vec4 tmp;

	float *v = &tmp.x;
	for (int i = 0; i < 4; i++)
	{
		v[i] = m[i + 0] * other.x +
			m[i + 4] * other.y +
			m[i + 8] * other.z +
			m[i + 12] * other.w;
	}

	return tmp;
}

Mat4 Mat4::operator* (const Mat4 &other) {
	const Mat4 *srcA = this;
	const Mat4 *srcB = &other;
	Mat4 tmp;

	for (int i = 0; i < 4; i++) {
		int a = 4 * i;
		int b = a + 1;
		int c = a + 2;
		int d = a + 3;

		tmp.m[a] = srcA->m[a] * srcB->m[0] +
			srcA->m[b] * srcB->m[4] +
			srcA->m[c] * srcB->m[8] +
			srcA->m[d] * srcB->m[12];

		tmp.m[b] = srcA->m[a] * srcB->m[1] +
			srcA->m[b] * srcB->m[5] +
			srcA->m[c] * srcB->m[9] +
			srcA->m[d] * srcB->m[13];

		tmp.m[c] = srcA->m[a] * srcB->m[2] +
			srcA->m[b] * srcB->m[6] +
			srcA->m[c] * srcB->m[10] +
			srcA->m[d] * srcB->m[14];

		tmp.m[d] = srcA->m[a] * srcB->m[3] +
			srcA->m[b] * srcB->m[7] +
			srcA->m[c] * srcB->m[11] +
			srcA->m[d] * srcB->m[15];
	}

	return tmp;
}

void Mat4::scale(Vec3 s) {
	m[0] *= s.x;
	m[1] *= s.x;
	m[2] *= s.x;
	m[3] *= s.x;

	m[4] *= s.y;
	m[5] *= s.y;
	m[6] *= s.y;
	m[7] *= s.y;

	m[8] *= s.z;
	m[9] *= s.z;
	m[10] *= s.z;
	m[11] *= s.z;
}

void Mat4::translate(Vec3 t) {
	m[12] += (m[0] * t.x + m[4] * t.y + m[8] * t.z);
	m[13] += (m[1] * t.x + m[5] * t.y + m[9] * t.z);
	m[14] += (m[2] * t.x + m[6] * t.y + m[10] * t.z);
	m[15] += (m[3] * t.x + m[7] * t.y + m[11] * t.z);
}

void Mat4::rotate(Vec4 q) {
	Mat4 rotationMatrix;
	rotationMatrix.m[0] = 1 - 2 * q.y * q.y - 2 * q.z * q.z;
	rotationMatrix.m[1] = 2 * q.x * q.y + 2 * q.w * q.z;
	rotationMatrix.m[2] = 2 * q.x * q.z - 2 * q.w * q.y;
	rotationMatrix.m[3] = 0.0f;

	rotationMatrix.m[4] = 2 * q.x * q.y - 2 * q.w * q.z;
	rotationMatrix.m[5] = 1 - 2 * q.x * q.x - 2 * q.z * q.z;
	rotationMatrix.m[6] = 2 * q.y * q.z + 2 * q.w * q.x;
	rotationMatrix.m[7] = 0.0f;

	rotationMatrix.m[8] = 2 * q.x * q.z + 2 * q.w * q.y;
	rotationMatrix.m[9] = 2 * q.y * q.z - 2 * q.w * q.x;
	rotationMatrix.m[10] = 1 - 2 * q.x * q.x - 2 * q.y * q.y;
	rotationMatrix.m[11] = 0.0f;

	rotationMatrix.m[12] = 0.0f;
	rotationMatrix.m[13] = 0.0f;
	rotationMatrix.m[14] = 0.0f;
	rotationMatrix.m[15] = 1.0f;

	multiply(rotationMatrix);
}

void Mat4::rotate(float angle, Vec3 a) {
	float sinAngle = sinf(angle * PI / 180.0f);
	float cosAngle = cosf(angle * PI / 180.0f);
	float oneMinusCos = 1.0f - cosAngle;
	float mag = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);

	if (mag != 0.0f && mag != 1.0f) {
		a.x /= mag;
		a.y /= mag;
		a.z /= mag;
	}

	float xx = a.x * a.x;
	float yy = a.y * a.y;
	float zz = a.z * a.z;
	float xy = a.x * a.y;
	float yz = a.y * a.z;
	float zx = a.z * a.x;
	float xs = a.x * sinAngle;
	float ys = a.y * sinAngle;
	float zs = a.z * sinAngle;

	Mat4 rotationMatrix;

	rotationMatrix.m[0] = (oneMinusCos * xx) + cosAngle;
	rotationMatrix.m[1] = (oneMinusCos * xy) - zs;
	rotationMatrix.m[2] = (oneMinusCos * zx) + ys;
	rotationMatrix.m[3] = 0.0f;

	rotationMatrix.m[4] = (oneMinusCos * xy) + zs;
	rotationMatrix.m[5] = (oneMinusCos * yy) + cosAngle;
	rotationMatrix.m[6] = (oneMinusCos * yz) - xs;
	rotationMatrix.m[7] = 0.0f;

	rotationMatrix.m[8] = (oneMinusCos * zx) - ys;
	rotationMatrix.m[9] = (oneMinusCos * yz) + xs;
	rotationMatrix.m[10] = (oneMinusCos * zz) + cosAngle;
	rotationMatrix.m[11] = 0.0f;

	rotationMatrix.m[12] = 0.0f;
	rotationMatrix.m[13] = 0.0f;
	rotationMatrix.m[14] = 0.0f;
	rotationMatrix.m[15] = 1.0f;

	multiply(rotationMatrix);
}

void Mat4::lookat(Vec3 eye, Vec3 at, Vec3 up) {
	Mat4 m;
	Vec3 forward, side;
	//------------------
	forward[0] = at.x - eye.x;
	forward[1] = at.y - eye.y;
	forward[2] = at.z - eye.z;

	forward.normalize();

	//------------------
	//Side = forward x up
	side = forward.cross(up).normalized();
	//------------------
	//Recompute up as: up = side x forward
	up = side.cross(forward);
	//------------------
	m.m[0] = side[0];
	m.m[4] = side[1];
	m.m[8] = side[2];
	m.m[12] = 0.0;
	//------------------
	m.m[1] = up[0];
	m.m[5] = up[1];
	m.m[9] = up[2];
	m.m[13] = 0.0;
	//------------------
	m.m[2] = -forward[0];
	m.m[6] = -forward[1];
	m.m[10] = -forward[2];
	m.m[14] = 0.0;
	//------------------
	m.m[3] = m.m[7] = m.m[11] = 0.0;
	m.m[15] = 1.0;

	multiply(m);
	translate(eye * -1.0f);
}

void Mat4::frustum(float left, float right, float bottom, float top, float nearZ, float farZ) {
	float deltaX = right - left;
	float deltaY = top - bottom;
	float deltaZ = farZ - nearZ;
	Mat4 frust;

	if ((nearZ <= 0.0f) || (farZ <= 0.0f) || (deltaX <= 0.0f) || (deltaY <= 0.0f) || (deltaZ <= 0.0f)) {
		std::cout << "Invalid frustrum" << std::endl;
		return;
	}

	frust.m[0] = 2.0f * nearZ / deltaX;
	frust.m[1] = frust.m[2] = frust.m[3] = 0.0f;

	frust.m[5] = 2.0f * nearZ / deltaY;
	frust.m[4] = frust.m[6] = frust.m[7] = 0.0f;

	frust.m[8] = (right + left) / deltaX;
	frust.m[9] = (top + bottom) / deltaY;
	frust.m[10] = -(nearZ + farZ) / deltaZ;
	frust.m[11] = -1.0f;

	frust.m[14] = -2.0f * nearZ * farZ / deltaZ;
	frust.m[12] = frust.m[13] = frust.m[15] = 0.0f;

	multiply(frust);
}

void Mat4::perspective(float fovY, float aspect, float _near, float _far) {
	float frustumHeight = tanf(fovY / 360 * PI) * _near;
	float frustumWidth = frustumHeight * aspect;

	frustum(-frustumWidth, frustumWidth, -frustumHeight, frustumHeight, _near, _far);
}

void Mat4::ortho(float left, float right, float bottom, float top, float nearZ, float farZ) {
	float deltaX = right - left;
	float deltaY = top - bottom;
	float deltaZ = farZ - nearZ;
	Mat4 ortho;

	if ((deltaX == 0) || (deltaY == 0) || (deltaZ == 0)) {
		std::cout << "Invalid ortho" << std::endl;
		return;
	}

	ortho.identity();
	ortho.m[0] = 2 / deltaX;
	ortho.m[12] = -(right + left) / deltaX;
	ortho.m[5] = 2 / deltaY;
	ortho.m[13] = -(top + bottom) / deltaY;
	ortho.m[10] = -2 / deltaZ;
	ortho.m[14] = -(nearZ + farZ) / deltaZ;

	multiply(ortho);
}

void Mat4::transpose() {
	float tmp[16];

	tmp[0] = m[0];
	tmp[1] = m[4];
	tmp[2] = m[8];
	tmp[3] = m[12];
	tmp[4] = m[1];
	tmp[5] = m[5];
	tmp[6] = m[9];
	tmp[7] = m[13];
	tmp[8] = m[2];
	tmp[9] = m[6];
	tmp[10] = m[10];
	tmp[11] = m[14];
	tmp[12] = m[3];
	tmp[13] = m[7];
	tmp[14] = m[11];
	tmp[15] = m[15];

	memcpy(m, tmp, sizeof(m));
}

void Mat4::inverse() {
	Mat4 *src = this;
	Mat4 *result = this;

	int swap;
	float t;
	float temp[4][4];

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			temp[i][j] = (*src)(i, j);
		}
	}

	identity();

	for (int i = 0; i < 4; i++) {
		swap = i;
		for (int j = i + 1; j < 4; j++) {
			if (fabs(temp[j][i]) > fabs(temp[i][i])) {
				swap = j;
			}
		}

		if (swap != i) {
			for (int k = 0; k < 4; k++) {
				t = temp[i][k];
				temp[i][k] = temp[swap][k];
				temp[swap][k] = t;

				t = (*result)(i, k);
				(*result)(i, k) = (*result)(swap, k);
				(*result)(swap, k) = t;
			}
		}
		if (temp[i][i] == 0) {
			std::cout << "Matrix is singular, cannot invert" << std::endl;
			return;
		}
		t = temp[i][i];
		for (int k = 0; k < 4; k++) {
			temp[i][k] /= t;
			(*result)(i, k) /= t;
		}
		for (int j = 0; j < 4; j++) {
			if (j != i) {
				t = temp[j][i];
				for (int k = 0; k < 4; k++) {
					temp[j][k] -= temp[i][k] * t;
					(*result)(j, k) -= (*result)(i, k) * t;
				}
			}
		}
	}
}

void Mat4::identity() {
	memset(m, 0, sizeof(m));
	m[0] = 1;
	m[5] = 1;
	m[10] = 1;
	m[15] = 1;
}

void Mat4::multiply(const Mat4 &with) {
	const Mat4 *srcA = &with;
	const Mat4 *srcB = this;
	float tmp[16];

	for (int i = 0; i < 4; i++) {
		int a = 4 * i;
		int b = a + 1;
		int c = a + 2;
		int d = a + 3;

		tmp[a] = srcA->m[a] * srcB->m[0] +
			srcA->m[b] * srcB->m[4] +
			srcA->m[c] * srcB->m[8] +
			srcA->m[d] * srcB->m[12];

		tmp[b] = srcA->m[a] * srcB->m[1] +
			srcA->m[b] * srcB->m[5] +
			srcA->m[c] * srcB->m[9] +
			srcA->m[d] * srcB->m[13];

		tmp[c] = srcA->m[a] * srcB->m[2] +
			srcA->m[b] * srcB->m[6] +
			srcA->m[c] * srcB->m[10] +
			srcA->m[d] * srcB->m[14];

		tmp[d] = srcA->m[a] * srcB->m[3] +
			srcA->m[b] * srcB->m[7] +
			srcA->m[c] * srcB->m[11] +
			srcA->m[d] * srcB->m[15];
	}

	memcpy(m, tmp, sizeof(m));
}

void Mat4::interpolate(Mat4& other, float x) {
	for (int i = 0; i<16; ++i) {
		m[i] = m[i] * (1.0f - x) + other.m[i] * x;
	}
}

Vec3 Mat4::unproject(Vec3 _v, float* viewport) {
	Mat4 i = *this;
	i.inverse();

	Vec4 v(_v.x, _v.y, _v.z, 1.0f);
	v.x = (v.x - viewport[0]) / viewport[2];
	v.y = (v.y - viewport[1]) / viewport[3];
	v.x = v.x * 2.0f - 1.0f;
	v.y = v.y * 2.0f - 1.0f;
	v.z = v.z * 2.0f - 1.0f;
	v.y *= -1;

	Vec4 o = i * v;
	float factor = 1.0f / o.w;

	return Vec3(o.x*factor, o.y*factor, o.z*factor);
}

Vec2 Mat4::project(Vec3 _v, float* viewport) {
	Vec4 v(_v.x, _v.y, _v.z, 1.0f);
	Vec4 o = (*this) * v;

	return Vec2(lerp<float>((o.x / o.w)*0.5f + 0.5f, viewport[0], viewport[2]),
		lerp<float>((o.y / o.w)*0.5f + 0.5f, viewport[3], viewport[1]));
}

void Mat4::setRow(int r, Vec4 val)
{
	m[r*4 + 0] = val.x;
	m[r*4 + 1] = val.y;
	m[r*4 + 2] = val.z;
	m[r*4 + 3] = val.w;
}
