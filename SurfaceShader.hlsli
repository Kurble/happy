#include "GBufferCommon.hlsli"

struct GBufferOut
{
	float4 graphicsBuffer0 : SV_Target0;
	float3 graphicsBuffer1 : SV_Target1;
	float4 graphicsBuffer2 : SV_Target2;
	float2 velocityBuffer : SV_Target3;
};

SamplerState g_TextureSampler : register(s0);

Texture2D<float4> g_MultiTexture0 : register(t0);
Texture2D<float3> g_MultiTexture1 : register(t1);
Texture2D<float4> g_MultiTexture2 : register(t2);

float2 clipSpaceToTextureSpace(float4 clipSpace)
{
	float2 cs = clipSpace.xy / clipSpace.w;
	return float2(0.5f * cs.x, -0.5f * cs.y) + 0.5f;
}

struct SurfaceShaderOut
{
	float3 diffuse;
	float  emissive;
	float3 normal;
	float3 specular;
	float  gloss;
};

SurfaceShaderOut surf_shader(VSOut input);

GBufferOut main(VSOut input)
{
	GBufferOut output;

	SurfaceShaderOut surfaceOut = surf_shader(input);
	
	float3x3 normalTransform = float3x3(input.tangent, input.binormal, input.normal);
	float3 normal = normalize(mul(surfaceOut.normal.xyz, normalTransform));

	output.graphicsBuffer0 = float4(surfaceOut.diffuse, surfaceOut.emissive);
	output.graphicsBuffer1 = float3(normal*0.5f + 0.5f);
	output.graphicsBuffer2 = float4(surfaceOut.specular, surfaceOut.gloss);
	output.velocityBuffer = clipSpaceToTextureSpace(input.currentPosition) - clipSpaceToTextureSpace(input.previousPosition);

	return output;
}