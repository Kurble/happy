#include "RendererCommon.hlsli"
#include "Utils.hlsli"

cbuffer CBufferSSAO: register(b2)
{
	float  g_OcclusionRadius;
	int    g_Samples;
	float  g_InvSamples;

	float3 g_SampleKernel[512];
};

static const uint  g_uNoiseSize = 64;
static const float g_fNoiseSize = 64.0f;

SamplerState       g_ScreenSampler : register(s0);

Texture2D<float4>  g_NormalBuffer  : register(t1);
Texture2D<float>   g_DepthBuffer   : register(t5);
Texture2D<float2>  g_Noise         : register(t6);

float3 sampleNoise(float2 pos)
{
	uint u = (uint)pos.x % g_uNoiseSize;
	uint v = (uint)pos.y % g_uNoiseSize;
	float2 xy = g_Noise.Sample(g_ScreenSampler, float2(u, v) / g_fNoiseSize).xy * 2.0f - 1.0f;

	return float3(xy, 0.0f);
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
	float3   bitangent = normalize(cross(normal, tangent));
	float3x3 tbn       = float3x3(tangent, bitangent, normal);

	//-----------------------------------------------------------------------------------------------
	// Calculate occlusion
	float occlusion = 0.0f;
	for (int i = 0; i < g_Samples; ++i) 
	{
		// calculate samplePosition
		float3 samplePosition = mul(g_SampleKernel[i], tbn);
		//float  tangentialCheck = dot(normalize(samplePosition), normal) > 0.2f ? 1.0f : 0.0f;
		samplePosition = originPosition + samplePosition * g_OcclusionRadius;

		float occlusionRadius = dot(samplePosition * g_OcclusionRadius, samplePosition * g_OcclusionRadius);

		// project samplePosition (so we know where to sample)
		float4 offset = float4(samplePosition, 1.0);
		offset = mul(jitteredView, offset);
		offset = mul(jitteredProjection, offset);
		offset.xy /= offset.w;
		offset.xy  = offset.xy * float2(0.5f, -0.5f) + 0.5f;

		float predictedDepth = offset.z / offset.w;

		float sampleDepth = g_DepthBuffer.Sample(g_ScreenSampler, offset.xy);
		float sampleLinearDepth = calcLinearDepth(offset.xy, sampleDepth);

		// range check and accumulate
		float rangeCheck = (originLinearDepth - sampleLinearDepth)*(originLinearDepth - sampleLinearDepth) < occlusionRadius ? 1.0 : 0.0;
		float depthCheck = sampleDepth <= predictedDepth ? 1.0f : 0.0;
		occlusion += depthCheck * rangeCheck;
	}

	occlusion = occlusion * g_InvSamples;
	return occlusion * 0.7f;
}