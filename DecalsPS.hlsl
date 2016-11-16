#include "GBufferCommon.h"

struct GBufferOut
{
	float4 graphicsBuffer0 : SV_Target0;
	float4 graphicsBuffer1 : SV_Target1;
	float4 graphicsBuffer2 : SV_Target2;
};

SamplerState g_TextureSampler : register(s0);

Texture2D<float4> g_MultiTexture0 : register(t0);
Texture2D<float4> g_MultiTexture1 : register(t1);
Texture2D<float4> g_MultiTexture2 : register(t2);
Texture2D<float>  g_DepthBuffer     : register(t5);

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
		output.graphicsBuffer0 = g_MultiTexture0.Sample(g_TextureSampler, 0.5f+cubePos.xy*0.5f);
		output.graphicsBuffer1 = g_MultiTexture1.Sample(g_TextureSampler, 0.5f+cubePos.xy*0.5f);
		output.graphicsBuffer2 = g_MultiTexture2.Sample(g_TextureSampler, 0.5f+cubePos.xy*0.5f);
	}
	else
	{
		output.graphicsBuffer0 = float4(0, 0, 0, 0);
		output.graphicsBuffer1 = float4(0, 0, 0, 0);
		output.graphicsBuffer2 = float4(0, 0, 0, 0);
	}

	return output;
}