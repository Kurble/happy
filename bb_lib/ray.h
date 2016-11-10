#pragma once

#include "vec3.h"

namespace bb
{
	struct ray
	{
		vec3 p0, p1;

		vec3 t(float _t) 
		{
			return p0 + (p1 - p0)*_t;
		}
	};
}