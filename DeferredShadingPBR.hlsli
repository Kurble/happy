#include "RendererCommon.h"

SamplerState             g_ScreenSampler   : register(s0);
SamplerState             g_TextureSampler  : register(s1);

Texture2D<float4>        g_GraphicsBuffer0 : register(t0);
Texture2D<float3>        g_GraphicsBuffer1 : register(t1);
Texture2D<float4>        g_GraphicsBuffer2 : register(t2);
Texture2D<float>         g_OcclusionBuffer : register(t4);
Texture2D<float>         g_DepthBuffer     : register(t5);

TextureCubeArray<float4> g_CubeLighting    : register(t6);
TextureCube<float4>      g_CubeEnvironment : register(t7);

float3 sampleEnv(float3 normal, float gloss)
{
	return g_CubeLighting.Sample(g_TextureSampler, float4(normal.xzy, round((convolutionStages - 1) * gloss))).rgb;
}

float4 main(VSOut input) : SV_TARGET
{
	//------------------------------------------------------------------------------------
	// Sampling
	float4 channel0  = g_GraphicsBuffer0.Sample(g_ScreenSampler, input.tex);
	float3 channel1  = g_GraphicsBuffer1.Sample(g_ScreenSampler, input.tex);
	float4 channel2  = g_GraphicsBuffer2.Sample(g_ScreenSampler, input.tex);
	float  occlusion = g_OcclusionBuffer.Sample(g_ScreenSampler, input.tex);
	float  depth     = g_DepthBuffer.Sample(g_ScreenSampler, input.tex);

	//------------------------------------------------------------------------------------
	// Unpack data from samples
	// 0: (albedo.rgb, emissive factor)
	// 1: (normal.xyz)
	// 2: (specular.rgb, gloss)
	float3 albedo   = channel0.rgb;
	float  emissive = channel0.a;
	float3 normal   = normalize(channel1.rgb * 2.0f - 1.0f);
	float3 specular = channel2.rgb;
	float  gloss    = channel2.a;

	//------------------------------------------------------------------------------------
	// Calculate pixel properties
	float3 screenNormal = float3((input.tex.x - 0.5f) *  2.0f * (width / height), 
		                         (input.tex.y - 0.5f) * -2.0f, 
		                         ( -projection[0][0]) *  2.0f);
	float3 viewNormal = normalize(mul((float3x3)viewInverse, screenNormal));

	//------------------------------------------------------------------------------------
	// Perform shading
	if (depth < 1)
	{
		float  schlick = pow(1 - max(0, dot(normal, -viewNormal)), 5.0f);

		// calculate diffuse part
		float3 diffContrib = (1.0f - schlick) * albedo * (1.0f - occlusion);
		float3 diffResult = sampleEnv(normal, 0) * 2.0f * diffContrib;

		// calculate specular part
		float3 specContrib = (specular + (1.0f - specular) * schlick);
		float3 specResult = sampleEnv(reflect(viewNormal, normal), gloss) * specContrib;

		return float4(diffResult + specResult, 1.0f);
		//return float4(normal * 0.5f + 0.5f, 1.0f);
	}
	else
	{
		return float4(0.0f, 0.0f, 0.0f, 1.0f);
	}
}