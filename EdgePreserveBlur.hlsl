#include "RendererCommon.h"

cbuffer CBufferDSSDO : register(b2)
{
	float  g_OcclusionRadius = 0.013;
	float  g_OcclusionMaxDistance = 0.13;
	float2 g_BlurDir;
	int    g_Samples = 32;
};

cbuffer CBufferRandom : register(b3)
{
	float4 random_points[128];
}

SamplerState g_ScreenSampler : register(s0);

Texture2D<float4> g_NormalBuffer : register(t1);
Texture2D<float4> g_OcclusionBuffer : register(t2);

float4 main(VSOut input) : SV_Target
{
	float weights[9] =
	{
		0.013519569015984728,
		0.047662179108871855,
		0.11723004402070096,
		0.20116755999375591,
		0.240841295721373,
		0.20116755999375591,
		0.11723004402070096,
		0.047662179108871855,
		0.013519569015984728
	};

	float indices[9] = { -4, -3, -2, -1, 0, +1, +2, +3, +4 };

	float2 step = g_BlurDir / float2(width, height);

	float3 normal[9];

	normal[0] = g_NormalBuffer.Sample(g_ScreenSampler, input.tex + indices[0] * step).xyz * 2 - 1;
	normal[1] = g_NormalBuffer.Sample(g_ScreenSampler, input.tex + indices[1] * step).xyz * 2 - 1;
	normal[2] = g_NormalBuffer.Sample(g_ScreenSampler, input.tex + indices[2] * step).xyz * 2 - 1;
	normal[3] = g_NormalBuffer.Sample(g_ScreenSampler, input.tex + indices[3] * step).xyz * 2 - 1;
	normal[4] = g_NormalBuffer.Sample(g_ScreenSampler, input.tex + indices[4] * step).xyz * 2 - 1;
	normal[5] = g_NormalBuffer.Sample(g_ScreenSampler, input.tex + indices[5] * step).xyz * 2 - 1;
	normal[6] = g_NormalBuffer.Sample(g_ScreenSampler, input.tex + indices[6] * step).xyz * 2 - 1;
	normal[7] = g_NormalBuffer.Sample(g_ScreenSampler, input.tex + indices[7] * step).xyz * 2 - 1;
	normal[8] = g_NormalBuffer.Sample(g_ScreenSampler, input.tex + indices[8] * step).xyz * 2 - 1;

	float total_weight = 1.0;
	float discard_threshold = 0.85;

	int i;

	for (i = 0; i < 9; ++i)
	{
		if (dot(normal[i], normal[4]) < discard_threshold)
		{
			total_weight -= weights[i];
			weights[i] = 0;
		}
	}

	//--------------------------------------------

	float4 res = 0;

	for (i = 0; i < 9; ++i)
	{
		res += g_OcclusionBuffer.Sample(g_ScreenSampler, input.tex + indices[i] * step) * weights[i];
	}

	res /= total_weight;

	return res;
}