#include "GBufferCommon.h"

struct GBufferOut
{
	float4 albedoRoughness  : SV_Target0;
	float4 normalMetallic : SV_Target1;
};

SamplerState g_TextureSampler : register(s0);

Texture2D<float4> g_AlbedoRoughnessMap : register(t0);
Texture2D<float4> g_NormalMetallicMap  : register(t1);
Texture2D<float>  g_DepthBuffer        : register(t2);

float3 samplePosition(float2 tex)
{
	float4 position = float4(
		tex.x * ( 2) - 1,
		tex.y * (-2) + 1,
		g_DepthBuffer.Sample(g_TextureSampler, tex),
		1.0f
	);

	position = mul(projectionInverse, position);
	position = mul(viewInverse, position);
	position = mul(worldInverse, position);

	return position.xyz / position.w;
}

GBufferOut main(VSOut input)
{
	GBufferOut output;

	float2 screenPos = input.position.xy / float2(width, height);

	float3 cubePos = samplePosition(screenPos);
	float3 checkPos = abs(cubePos);

	if (checkPos.x < 1 &&
		checkPos.y < 1 &&
		checkPos.z < 1)
	{
		output.albedoRoughness = g_AlbedoRoughnessMap.Sample(g_TextureSampler, 0.5f+cubePos.xy*0.5f);
		output.normalMetallic = g_NormalMetallicMap.Sample(g_TextureSampler, 0.5f+cubePos.xy*0.5f);
	}
	else
	{
		output.albedoRoughness = float4(0, 0, 0, 0);
		output.normalMetallic = float4(0, 0, 0, 0);
	}

	return output;
}