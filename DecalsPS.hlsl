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
Texture2D<uint>   g_StencilBuffer      : register(t3);

float3 samplePosition(float2 tex)
{
	float4 position = float4(
		tex.x * (2) - 1,
		tex.y * (-2) + 1,
		g_DepthBuffer.Sample(g_TextureSampler, tex),
		1.0f
		);

	position = mul(projectionInverse, position);
	position = mul(viewInverse, position);

	return position.xyz / position.w;
}

GBufferOut main(VSOut input)
{
	GBufferOut output;

	float3x3 normalMatrix = float3x3(
		input.tangent,
		input.binormal,
		input.normal
	);

	output.albedoRoughness = g_AlbedoRoughnessMap.Sample(g_TextureSampler, input.texcoord0);
	float4 normalMetallic = g_NormalMetallicMap.Sample(g_TextureSampler, input.texcoord0);
	float3 normal = normalize(mul(2.0f*normalMetallic.xyz - 1.0f, normalMatrix));
	float  metallic = normalMetallic.w;
	output.normalMetallic = float4(normal.x*0.5f + 0.5f, normal.y*0.5f + 0.5f, normal.z*0.5f + 0.5f, metallic);

	return output;
}