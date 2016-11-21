#include "RendererCommon.h"
#include "Utils.hlsli"

cbuffer CBufferSSAO: register(b2)
{
	float  g_OcclusionRadius;
	int    g_Samples;
	float  g_InvSamples;

	float3 g_SampleKernel[512];
};

static const uint  g_uNoiseSize = 4;
static const float g_fNoiseSize = 4.0f;

SamplerState       g_ScreenSampler : register(s0);

Texture2D<float4>  g_NormalBuffer  : register(t1);
Texture2D<float>   g_DepthBuffer   : register(t5);
Texture2D<float4>  g_Noise         : register(t6);

float3 sampleNoise(float2 pos)
{
	uint u = (uint)pos.x % g_uNoiseSize;
	uint v = (uint)pos.y % g_uNoiseSize;
	return g_Noise.Sample(g_ScreenSampler, float2(u, v) / g_fNoiseSize).xyz * 2.0f - 1.0f;
}

float main(VSOut input) : SV_TARGET
{
	float  originDepth = g_DepthBuffer.Sample(g_ScreenSampler, input.tex);
	float3 originPosition = calcPosition(input.tex, originDepth);
	float  originLinearDepth = calcLinearDepth(input.tex, originDepth);

	//-----------------------------------------------------------------------------------------------
	// Calculate TBN matrix
	float3   noise     = sampleNoise(input.pos.xy);
	float3   normal    = normalize(g_NormalBuffer.Sample(g_ScreenSampler, input.tex).xyz * 2.0f - 1.0f);
	float3   tangent   = normalize(noise - normal * dot(noise, normal));
	float3   bitangent = cross(normal, tangent);
	float3x3 tbn       = float3x3(tangent, bitangent, normal);

	//-----------------------------------------------------------------------------------------------
	// Calculate occlusion
	float occlusion = 0.0f;
	for (int i = 0; i < g_Samples; ++i) 
	{
		// calculate samplePosition
		float3 samplePosition = mul(g_SampleKernel[i], tbn);
		samplePosition = originPosition + samplePosition * g_OcclusionRadius;

		// project samplePosition (so we know where to sample)
		float4 offset = float4(samplePosition, 1.0);
		offset = mul(view, offset);
		offset = mul(projection, offset);
		offset.xy /= offset.w;
		offset.xy  = offset.xy * float2(0.5f, -0.5f) + 0.5f;

		float sampleDepth = g_DepthBuffer.Sample(g_ScreenSampler, offset.xy);
		float sampleLinearDepth = calcLinearDepth(offset.xy, sampleDepth);

		// range check and accumulate
		float rangeCheck = abs(originLinearDepth - sampleLinearDepth) < g_OcclusionRadius ? 1.0 : 0.0;
		occlusion += (sampleDepth <= originDepth ? 1.0 : 0.0) * rangeCheck;
	}

	return (occlusion / (float)g_Samples);
}