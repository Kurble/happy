#include "vec3.h"
#include "vec4.h"
#include "mat3.h"
#include "math_util.h"
#include <cmath>

namespace bb
{
	vec4::vec4() {}
	vec4::vec4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
	vec4::vec4(const vec3 &pos) : x(pos.x), y(pos.y), z(pos.z), w(1.0f) {}

	void vec4::set(const float x, const float y, const float z, const float w)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->w = w;
	}

	void vec4::setEuler(const float yaw, const float pitch, const float roll)
	{
		float c1 = cosf(yaw   * 0.5f);
		float c2 = cosf(roll  * 0.5f);
		float c3 = cosf(pitch * 0.5f);
		float s1 = sinf(yaw   * 0.5f);
		float s2 = sinf(roll  * 0.5f);
		float s3 = sinf(pitch * 0.5f);

		w = c1*c2*c3 - s1*s2*s3;
		x = s1*s2*c3 + c1*c2*s3;
		y = s1*c2*c3 + c1*s2*s3;
		z = c1*s2*c3 - s1*c2*s3;
	}

	float vec4::dot(const vec4& b) const
	{
		return x*b.x + y*b.y + z*b.z + w*b.w;
	}

	vec4 vec4::normalized() const
	{
		float length = 1.0f / sqrtf(x*x + y*y + z*z + w*w);
		vec4 r = { x*length, y*length, z*length, w*length };
		return r;
	}

	vec4 vec4::slerp(const float& x, const vec4& other) const
	{
		if (x <= 0.0f)
			return *this;
		if (x >= 1.0f)
			return other;

		vec4 self = *this;
		vec4 result = other;
		float c = dot(other);

		if (c < 0)
		{
			result = result * -1;
			c = -c;
		}

		if (c > 1.0f - epsilon)
			return lerp(self, result, x).normalized();

		float a = acosf(c);
		return (self * sinf((1.0f - x) * a) + result * sinf(x * a)) * (1.0f / sinf(a));
	}

	vec4 vec4::slerp(const float& x, const vec4& a, const vec4& b)
	{
		return a.slerp(x, b);
	}

	vec4 vec4::multiplyQuaternion(const vec4& other) const
	{
		vec4 res;
		const vec4& q = *this;
		const vec4& p = other;

		res.w = q.w * p.w - q.x * p.x - q.y * p.y - q.z * p.z;
		res.x = q.w * p.x + q.x * p.w + q.y * p.z - q.z * p.y;
		res.y = q.w * p.y + q.y * p.w + q.z * p.x - q.x * p.z;
		res.z = q.w * p.z + q.z * p.w + q.x * p.y - q.y * p.x;

		return res;
	}

	vec4 vec4::multiplyQuaternion(const vec4& a, const vec4& b)
	{
		return a.multiplyQuaternion(b);
	}

	vec4 vec4::rotationBetween(const vec3& a, const vec3& b)
	{
		vec3 axis = a.cross(b).normalized();
		float angle = acosf(a.dot(b));

		return vec4(axis.x * sinf(angle*0.5f),
			axis.y * sinf(angle*0.5f),
			axis.z * sinf(angle*0.5f),
			cosf(angle*0.5f));
	}

	vec4 vec4::rotationBetween(const vec3& a, const vec3& b, const vec3& axis, const bool snap)
	{
		float angle = acosf(a.dot(b));

		if (snap) {
			angle = 0.261799388f * floorf((angle / 0.261799388f) + 0.5f);
		}

		return vec4(axis.x * sinf(angle*0.5f),
			axis.y * sinf(angle*0.5f),
			axis.z * sinf(angle*0.5f),
			cosf(angle*0.5f));
	}

	vec4 vec4::lookAt(const vec3& source, const vec3& dest)
	{
		vec3 forward = (dest - source).normalized();

		float dot = vec3(0, 0, -1).dot(forward);

		if (fabsf(dot - (-1.0f)) < 0.000001f)
		{
			return vec4(0, 1, 0, 3.141592f);
		}
		if (fabsf(dot - (1.0f)) < 0.000001f)
		{
			return vec4(0, 0, 0, 1);
		}

		float rotAngle = cosf(dot);
		vec3 axis = vec3(0, 0, -1).cross(forward).normalized();

		return vec4(
			axis.x * sinf(rotAngle*0.5f),
			axis.y * sinf(rotAngle*0.5f),
			axis.z * sinf(rotAngle*0.5f),
			cosf(rotAngle*0.5f));
	}

	vec4 vec4::quatSnap(const vec4 &quat, const float snapAngle)
	{
		float angle = acosf(quat.w) * 2.0f;
		vec3 axis = 
		{
			quat.x / sinf(angle * 0.5f),
			quat.y / sinf(angle * 0.5f),
			quat.z / sinf(angle * 0.5f)
		};

		angle = (floorf(angle / snapAngle)) * snapAngle;

		return vec4(
			axis.x * sinf(angle*0.5f),
			axis.y * sinf(angle*0.5f),
			axis.z * sinf(angle*0.5f),
			cosf(angle*0.5f));
	}

	bool vec4::operator==(const vec4 &b) const
	{
		return (b.x == x && b.y == y && b.z == z && b.w == w);
	}

	bool vec4::operator <(const vec4 &b) const
	{
		if (x == b.x)
			if (y == b.y)
				if (z == b.z)
					return w < b.w;
				else
					return z < b.z;
			else
				return y < b.y;
		else
			return x < b.x;
				
	}

	vec3 vec4::operator*(const vec3& b)
	{
		mat3 rotation;
		rotation.identity();
		rotation.rotate(*this);

		return rotation * b;
	}

	vec4 vec4::operator*(const float& scalar) const
	{
		return{ x*scalar, y*scalar, z*scalar, w*scalar };
	}

	vec4 vec4::operator+(const vec4& b) const
	{
		return{ x + b.x, y + b.y, z + b.z, w + b.w };
	}

	vec4 vec4::operator-(const vec4& b) const
	{
		return{ x - b.x, y - b.y, z - b.z, w - b.w };
	}

	float& vec4::operator[](int i) 
	{
		return (&x)[i];
	}
}