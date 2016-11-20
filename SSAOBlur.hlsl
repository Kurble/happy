#include "RendererCommon.h"

SamplerState       g_ScreenSampler   : register(s0);
Texture2D<float>   g_OcclusionBuffer : register(t6);

static const uint  g_uNoiseSize = 4;
static const float g_fNoiseSize = 4.0f;

float main(VSOut input) : SV_TARGET
{
	float2 texel = 1.0f / float2(width, height);

	float  result = 0.0f;
	float2 offset = (-g_fNoiseSize * 0.5f + 0.5f) / float2(width, height);
	for (float i = 0; i < g_fNoiseSize; ++i)
	{
		for (float j = 0; j < g_fNoiseSize; ++j)
		{
			float2 tex = input.tex + float2(
				offset.x + i * texel.x,
				offset.y + j * texel.y
			);
			result += g_OcclusionBuffer.Sample(g_ScreenSampler, tex);
		}
	}

	return result / (g_fNoiseSize*g_fNoiseSize);
}