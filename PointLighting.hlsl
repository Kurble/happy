#include "CBuffers.h"

struct VSOut
{
	float4 position : SV_Position;
	float2 tex : TEXCOORD0;
};

cbuffer CBufferPointLight : register(b1)
{
	float4 lightPosition;
	float4 lightColor;
	float lightSize;
	float lightFalloff;
}

SamplerState g_ScreenSampler  : register(s0);
SamplerState g_TextureSampler : register(s1);

Texture2D<float4> g_AlbedoRoughnessBuffer    : register(t0);
Texture2D<float4> g_NormalMetallicBuffer     : register(t1);
Texture2D<float4> g_OcclusionBuffer          : register(t2);
Texture2D<float>  g_DepthBuffer              : register(t3);

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
	float2 screenTex = input.position.xy / float2(width, height);

	// Fetch material data from G-Buffer
	float4 albedoRoughness = g_AlbedoRoughnessBuffer.Sample(g_ScreenSampler, screenTex);
	float4 normalMetallic  = g_NormalMetallicBuffer.Sample(g_ScreenSampler, screenTex);
	float4 occlusion       = g_OcclusionBuffer.Sample(g_ScreenSampler, screenTex) * 2 - 1;
	float  depth           = g_DepthBuffer.Sample(g_ScreenSampler, screenTex);

	// Calculate view normal
	float3 screenNormal = float3(
		(screenTex.x - .5) * 2 * (width / height),
		(screenTex.y - .5) * -2,
		-projection[0][0] * 2);
	float3 viewNormal = normalize(mul(screenNormal, (float3x3)view));

	if (depth < 1)
	{
		float3 position = samplePosition(screenTex);
		float3 to_light = lightPosition.xyz - position;
		float  to_light_dist = length(to_light);
		float3 to_light_norm = to_light / to_light_dist;
		float light_occlusion = 1 - saturate(dot(to_light_norm, occlusion.xyz));

		float intensity = saturate((lightSize - to_light_dist) / lightSize);
		
		float3 normal = normalMetallic.xyz * 2.0f - 1.0f;
		float rough = albedoRoughness.w;
		float invRough = 1 - rough;
		float invMetal = 1 - normalMetallic.w;
		float reflectivity = normalMetallic.w + invMetal * invRough * pow(1.0f + dot(viewNormal, normal), 4);
		float3 color = albedoRoughness.xyz * lightColor.xyz;

		float3 diffuse = saturate(dot(normal, to_light_norm)) * color * intensity;
		float3 specular = (pow(saturate(dot(reflect(viewNormal, normal), to_light_norm)), invRough *1000)*invRough) * color;

		return float4(lerp(diffuse, specular, reflectivity) * light_occlusion, 1.0f);
	}
	else
	{
		return 0;
	}
}