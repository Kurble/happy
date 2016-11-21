#include "GBufferCommon.h"

struct GBufferOut
{
	float4 graphicsBuffer0 : SV_Target0;
	float3 graphicsBuffer1 : SV_Target1;
	float4 graphicsBuffer2 : SV_Target2;
};

SamplerState g_TextureSampler : register(s0);

Texture2D<float4> g_MultiTexture0 : register(t0);
Texture2D<float3> g_MultiTexture1 : register(t1);
Texture2D<float4> g_MultiTexture2 : register(t2);

GBufferOut main(VSOut input)
{
	GBufferOut output;

	float3x3 normalTransform = float3x3(input.tangent, input.binormal, input.normal);

	output.graphicsBuffer0 = g_MultiTexture0.Sample(g_TextureSampler, input.texcoord0);
	float3 multi1 = g_MultiTexture1.Sample(g_TextureSampler, input.texcoord0);
	float3 normal = normalize(mul(multi1.xyz*2.0f-1.0f, normalTransform));
	output.graphicsBuffer1 = normal.xyz*0.5f+0.5f;
	output.graphicsBuffer2 = g_MultiTexture2.Sample(g_TextureSampler, input.texcoord0);

	return output;
}