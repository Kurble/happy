#include "RendererCommon.h"

SamplerState             g_ScreenSampler   : register(s0);
SamplerState             g_TextureSampler  : register(s1);
								           
Texture2D<float4>        g_GraphicsBuffer0 : register(t0);
Texture2D<float4>        g_GraphicsBuffer1 : register(t1);
Texture2D<float4>        g_GraphicsBuffer2 : register(t2);
Texture2D<float4>        g_OcclusionBuffer : register(t3);
Texture2D<float>         g_DepthBuffer     : register(t4);

TextureCubeArray<float4> g_CubeLighting    : register(t5);
TextureCube<float4>      g_CubeEnvironment : register(t6);

float3 sampleEnv(float3 normal, float gloss)
{
	return g_CubeLighting.Sample(g_TextureSampler, float4(normal.x, normal.z, normal.y, round((convolutionStages - 1) * gloss))).xyz;
}

float4 main(VSOut input) : SV_TARGET
{
	//------------------------------------------------------------------------------------
	// Sample data
	float4 channel0  = g_GraphicsBuffer0.Sample(g_ScreenSampler, input.tex);
	float4 channel1  = g_GraphicsBuffer1.Sample(g_ScreenSampler, input.tex);
	float4 channel2  = g_GraphicsBuffer2.Sample(g_ScreenSampler, input.tex);
	float4 occlusion = g_OcclusionBuffer.Sample(g_ScreenSampler, input.tex) * 2.0f - 1.0f;
	float  depth     = g_DepthBuffer.    Sample(g_ScreenSampler, input.tex);

	//------------------------------------------------------------------------------------
	// Unpack data from samples
	// 0:   (albedo.rgb, emissive factor)
	// 1:   (normal.xy, heightmap, gloss)
	// 2:   (specular.rgb, cavity)
	float3 albedo   = channel0.rgb;
	float  emissive = channel0.a;
	float3 normal   = float3(channel1.rg * 2.0f - 1.0f, 0.0f); 
	       normal.z = sqrt(1.0f - normal.x*normal.x - normal.y*normal.y);
	float  height   = channel1.b;
	float  gloss    = channel1.a;
	float3 specular = channel2.rgb;
	float  cavity   = channel2.a;

	//------------------------------------------------------------------------------------
	// Prepare more data
	float3 screenNormal = float3(
		(input.tex.x - .5) * 2 * (width / height),
		(input.tex.y - .5) * -2,
		-projection[0][0] * 2);
	float3 viewNormal = normalize(mul(screenNormal, (float3x3)view));

	//------------------------------------------------------------------------------------
	// Perform shading
	if (depth < 1)
	{
		// calculate diffuse part
		float3 diffuseResult = sampleEnv(normal, 0) * albedo;

		// calculate specular part
		float3 specularResult = sampleEnv(reflect(viewNormal, normal), gloss) * specular;

		return float4(diffuseResult + specularResult, 1.0f);
	}
	else
	{
		return float4(0.0f, 0.0f, 0.0f, 1.0f);
	}
}