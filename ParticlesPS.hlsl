#include "GBufferCommon.hlsli"
#include "Particles.hlsli"

float4 main(ParticleVertex input) : SV_TARGET
{
	return input.color;
}