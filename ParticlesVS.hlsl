#include "GBufferCommon.hlsli"
#include "Particles.hlsli"

ParticleVertex main(EmittedParticleVertex input)
{
	ParticleVertex output;

	output.position           = input.position;
	output.lifeSizeGrowWiggle = input.lifeSizeGrowWiggle;
	output.velocityFriction   = input.velocityFriction;  
	output.color              = input.color;          

	return output;
}