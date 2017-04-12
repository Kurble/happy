#include "geometry_util.h"
#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "mat3.h"

#include <exception>

namespace bb
{
	mat3 calculate_tbn(vec4* pos, vec2* uv)
	{
		mat3 result;

		vec2 uv1 = (uv[1] - uv[0]).normalized() * vec2(1, -1);
		vec2 uv2 = (uv[2] - uv[0]).normalized() * vec2(1, -1);

		float uvMatrix[] =
		{
			uv2.y, -uv1.y,
			-uv2.x,  uv1.x
		};
		float det = 1.0f / ((uv1.x * uv2.y) - (uv2.x * uv1.y));

		vec4 pos1 = pos[1] + (pos[0] * -1);
		vec4 pos2 = pos[2] + (pos[0] * -1);

		float posMatrix[] =
		{
			pos1.x, pos1.y, pos1.z,
			pos2.x, pos2.y, pos2.z
		};

		vec3 tangent = vec3(
			det * (uvMatrix[0] * posMatrix[0] + uvMatrix[1] * posMatrix[3]),
			det * (uvMatrix[0] * posMatrix[1] + uvMatrix[1] * posMatrix[4]),
			det * (uvMatrix[0] * posMatrix[2] + uvMatrix[1] * posMatrix[5]));
		vec3 binormal = vec3(
			det * (uvMatrix[2] * posMatrix[0] + uvMatrix[3] * posMatrix[3]),
			det * (uvMatrix[2] * posMatrix[1] + uvMatrix[3] * posMatrix[4]),
			det * (uvMatrix[2] * posMatrix[2] + uvMatrix[3] * posMatrix[5]));
		vec3 normal = vec3(pos2.x, pos2.y, pos2.z).cross(vec3(pos1.x, pos1.y, pos1.z));

		tangent.normalize();
		binormal.normalize();
		normal.normalize();

		result.setRow(0, tangent);
		result.setRow(1, binormal);
		result.setRow(2, normal);
		

		return result;
	}
}