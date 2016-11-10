#include "mat3.h"
#include "mat4.h"
#include "vec3.h"
#include "vec4.h"
#include <cmath>
#include <cstring>

namespace bb
{
	mat3::mat3() 
	{
		for (int i = 0; i < 9; i++) 
		{
			m[i] = 0;
		}
	}

	mat3::mat3(const mat3& other) 
	{
		for (int i = 0; i < 9; i++) 
		{
			m[i] = other.m[i];
		}
	}

	mat3::mat3(const mat4& other) 
	{
		m[0] = other.m[0];
		m[1] = other.m[1];
		m[2] = other.m[2];

		m[3] = other.m[4];
		m[4] = other.m[5];
		m[5] = other.m[6];

		m[6] = other.m[8];
		m[7] = other.m[9];
		m[8] = other.m[10];
	}

	float& mat3::operator()(int i, int j) 
	{
		return m[i * 3 + j];
	}

	float mat3::operator()(int i, int j) const 
	{
		return m[i * 3 + j];
	}

	mat3& mat3::operator= (const mat3 &other) 
	{
		for (int i = 0; i < 9; i++) 
		{
			m[i] = other.m[i];
		}

		return *this;
	}

	mat3& mat3::operator= (const float *other) 
	{
		for (int i = 0; i < 9; i++) 
		{
			m[i] = other[i];
		}

		return *this;
	}

	bool mat3::operator== (const mat3 &other) 
	{
		for (int i = 0; i < 9; i++) 
		{
			if (m[i] != other.m[i]) 
			{
				return false;
			}
		}

		return true;
	}

	vec3 mat3::operator* (const vec3 &other) 
	{
		vec3 tmp;

		float *v = &tmp.x;
		for (int i = 0; i < 3; i++)
		{
			v[i] = m[i + 0] * other.x +
				m[i + 3] * other.y +
				m[i + 6] * other.z;
		}

		return tmp;
	}

	mat3 mat3::operator* (const mat3 &other) 
	{
		const mat3 *srcA = this;
		const mat3 *srcB = &other;
		mat3 tmp;

		for (int i = 0; i < 3; i++)
		{
			int a = 3 * i;
			int b = a + 1;
			int c = a + 2;

			tmp.m[a] = srcA->m[a] * srcB->m[0] +
				srcA->m[b] * srcB->m[3] +
				srcA->m[c] * srcB->m[6];

			tmp.m[b] = srcA->m[a] * srcB->m[1] +
				srcA->m[b] * srcB->m[4] +
				srcA->m[c] * srcB->m[7];

			tmp.m[c] = srcA->m[a] * srcB->m[2] +
				srcA->m[b] * srcB->m[5] +
				srcA->m[c] * srcB->m[8];
		}

		return tmp;
	}

	void mat3::rotate(vec4 q) 
	{
		mat3 rotationMatrix;
		rotationMatrix.m[0] = 1 - 2 * q.y * q.y - 2 * q.z * q.z;
		rotationMatrix.m[1] = 2 * q.x * q.y + 2 * q.w * q.z;
		rotationMatrix.m[2] = 2 * q.x * q.z - 2 * q.w * q.y;

		rotationMatrix.m[3] = 2 * q.x * q.y - 2 * q.w * q.z;
		rotationMatrix.m[4] = 1 - 2 * q.x * q.x - 2 * q.z * q.z;
		rotationMatrix.m[5] = 2 * q.y * q.z + 2 * q.w * q.x;

		rotationMatrix.m[6] = 2 * q.x * q.z + 2 * q.w * q.y;
		rotationMatrix.m[7] = 2 * q.y * q.z - 2 * q.w * q.x;
		rotationMatrix.m[8] = 1 - 2 * q.x * q.x - 2 * q.y * q.y;

		multiply(rotationMatrix);
	}

	void mat3::transpose() 
	{
		float tmp[9];

		tmp[0] = m[0];
		tmp[1] = m[3];
		tmp[2] = m[6];
		tmp[3] = m[1];
		tmp[4] = m[4];
		tmp[5] = m[7];
		tmp[6] = m[2];
		tmp[7] = m[5];
		tmp[8] = m[8];

		memcpy(m, tmp, sizeof(m));
	}

	void mat3::adjoint() 
	{
		float a1 = m[0];
		float a2 = m[3];
		float a3 = m[6];

		float b1 = m[1];
		float b2 = m[4];
		float b3 = m[7];

		float c1 = m[2];
		float c2 = m[5];
		float c3 = m[8];

		m[0] = (b2*c3 - b3*c2);
		m[3] = (a3*c2 - a2*c3);
		m[6] = (a2*b3 - a3*b2);

		m[1] = (b3*c1 - b1*c3);
		m[4] = (a1*c3 - a3*c1);
		m[7] = (a3*b1 - a1*b3);

		m[2] = (b1*c2 - b2*c1);
		m[5] = (a2*c1 - a1*c2);
		m[8] = (a1*b2 - a2*b1);
	}

	void mat3::inverse() 
	{
		float a1 = m[0];
		float a2 = m[3];
		float a3 = m[6];

		float b1 = m[1];
		float b2 = m[4];
		float b3 = m[7];

		float c1 = m[2];
		float c2 = m[5];
		float c3 = m[8];

		float det = (a1*(b2*c3 - b3*c2) + a2*(b3*c1 - b1 * c3) + a3*(b1*c2 - b2*c1));

		m[0] = (b2*c3 - b3*c2) / det;
		m[3] = (a3*c2 - a2*c3) / det;
		m[6] = (a2*b3 - a3*b2) / det;

		m[1] = (b3*c1 - b1*c3) / det;
		m[4] = (a1*c3 - a3*c1) / det;
		m[7] = (a3*b1 - a1*b3) / det;

		m[2] = (b1*c2 - b2*c1) / det;
		m[5] = (a2*c1 - a1*c2) / det;
		m[8] = (a1*b2 - a2*b1) / det;
	}

	void mat3::identity() 
	{
		memset(m, 0, sizeof(m));
		m[0] = 1;
		m[4] = 1;
		m[8] = 1;
	}

	void mat3::setRow(int r, vec3 row)
	{
		m[r + 0] = row.x;
		m[r + 3] = row.y;
		m[r + 6] = row.z;
	}

	void mat3::multiply(const mat3 &with) 
	{
		const mat3 *srcA = &with;
		const mat3 *srcB = this;
		float tmp[9];

		for (int i = 0; i < 3; i++)
		{
			int a = 3 * i;
			int b = a + 1;
			int c = a + 2;

			tmp[a] = srcA->m[a] * srcB->m[0] +
				srcA->m[b] * srcB->m[3] +
				srcA->m[c] * srcB->m[6];

			tmp[b] = srcA->m[a] * srcB->m[1] +
				srcA->m[b] * srcB->m[4] +
				srcA->m[c] * srcB->m[7];

			tmp[c] = srcA->m[a] * srcB->m[2] +
				srcA->m[b] * srcB->m[5] +
				srcA->m[c] * srcB->m[8];
		}

		memcpy(m, tmp, sizeof(m));
	}
}