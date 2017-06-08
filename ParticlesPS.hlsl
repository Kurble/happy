#include "GBufferCommon.hlsli"
#include "Particles.hlsli"

float4 main(ParticleRenderVertex input) : SV_TARGET
{
	return input.color;
}