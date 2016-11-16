#include "GBufferCommon.h"

struct GBufferOut
{
	float4 graphicsBuffer0 : SV_Target0;
	float4 graphicsBuffer1 : SV_Target1;
	float4 graphicsBuffer2 : SV_Target2;
	float  graphicsBuffer3 : SV_Target3;
};

SamplerState g_TextureSampler : register(s0);

Texture2D<float4> g_MultiTexture0 : register(t0);
Texture2D<float4> g_MultiTexture1 : register(t1);
Texture2D<float4> g_MultiTexture2 : register(t2);
Texture2D<float>  g_MultiTexture3 : register(t3);

GBufferOut main(VSOut input)
{
	GBufferOut output;

	float3x3 normalMatrix = float3x3(
		input.tangent,
		input.binormal,
		input.normal
	);

	output.graphicsBuffer0 = g_MultiTexture0.Sample(g_TextureSampler, input.texcoord0);
	float4 multi1 = g_MultiTexture1.Sample(g_TextureSampler, input.texcoord0);
	float3 normal = normalize(mul(2.0f*multi1.xyz-1.0f, normalMatrix));
	float  gloss  = multi1.w;
	output.graphicsBuffer1 = float4(normal.xyz*0.5f+0.5f, gloss);
	output.graphicsBuffer2 = g_MultiTexture2.Sample(g_TextureSampler, input.texcoord0);
	output.graphicsBuffer3 = g_MultiTexture3.Sample(g_TextureSampler, input.texcoord0);

	return output;
}