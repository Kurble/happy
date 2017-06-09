#include "GBufferCommon.hlsli"
#include "Particles.hlsli"

SamplerState g_Sampler : register(s0);
Texture2D<float4> g_Texture : register(t0);

float4 main(ParticleRenderVertex input) : SV_TARGET
{
	return input.color * g_Texture.Sample(g_Sampler, input.texCoord);
}