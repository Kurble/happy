#include "RendererCommon.h"
//#include "DSSDO_Kernel.h"

cbuffer CBufferDSSDO : register(b2)
{
	float g_OcclusionRadius = 0.013;
	float g_OcclusionMaxDistance = 0.13;
	int   g_Samples = 32;

	float4 random_points[128];
};

static const float2 g_NoiseTextureSize = float2(4, 4);

SamplerState g_ScreenSampler  : register(s0);

Texture2D<float4> g_AlbedoRoughnessBuffer    : register(t0);
Texture2D<float4> g_NormalMetallicBuffer     : register(t1);
Texture2D<float>  g_DepthBuffer              : register(t3);
Texture2D<float4> g_Noise                    : register(t4);

float3 samplePosition(float2 tex)
{
	float4 position = float4(
		tex.x * ( 2) - 1,
		tex.y * (-2) + 1,
		g_DepthBuffer.Sample(g_ScreenSampler, tex),
		1.0f
	);

	position = mul(projectionInverse, position);
	position = mul(viewInverse, position);

	return position.xyz / position.w;
}

float4 main(VSOut input) : SV_TARGET
{
	float3 position = samplePosition(input.tex);
	float3 eye = float3(viewInverse[0][3],
		                viewInverse[1][3],
						viewInverse[2][3]);

	float depth = distance(eye, position.xyz);
	float radius = g_OcclusionRadius / depth;
	float max_distance_inv = 1.0f / g_OcclusionMaxDistance;
	float ettenuation_angle_treshold = 0.1f;

	float3 noise = g_Noise.Sample(g_ScreenSampler, input.tex*float2(width, height) / g_NoiseTextureSize).xyz * 2 - 1;

	float3 normal = g_NormalMetallicBuffer.Sample(g_ScreenSampler, input.tex).xyz * 2 - 1;
	float4 occlusion_sh2 = 0;

	static const float  fudge_factor_l0 = 2;
	static const float  fudge_factor_l1 = 10.0;
	static const float  sh2_weight_l0 = fudge_factor_l0 * 0.28209; //0.5*sqrt(1.0/pi);
	static const float3 sh2_weight_l1 = fudge_factor_l1 * 0.48860; //0.5*sqrt(3.0/pi);
	static const float attenuation_angle_threshold = 0.25;

	const float4 sh2_weight = float4(sh2_weight_l1, sh2_weight_l0) / g_Samples;

	[unroll] // compiler wants to branch here by default and this makes it run nearly 2x slower.
	for (int i = 0; i<g_Samples; ++i)
	{
		float2 textureOffset = reflect(random_points[i].xy, noise.xy).xy * radius;
		float2 sample_tex = input.tex + textureOffset;
		float3 sample_pos = samplePosition(sample_tex);
		float3 center_to_sample = sample_pos - position;
		float dist = length(center_to_sample);
		float3 center_to_sample_normalized = center_to_sample / dist;
		float attenuation = 1 - saturate(dist * max_distance_inv);
		float dp = dot(normal, center_to_sample_normalized);

		attenuation = attenuation*attenuation * step(attenuation_angle_threshold, dp);

		occlusion_sh2 += attenuation * sh2_weight*float4(center_to_sample_normalized, 1);
	}

	return occlusion_sh2 * .5 + .5;
}