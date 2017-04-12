#pragma once

namespace bb
{
	struct vec2;

	struct vec4;

	struct mat3;

	mat3 calculate_tbn(vec4* pos, vec2* uv);
}