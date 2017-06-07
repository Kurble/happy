#include "GBufferCommon.hlsli"
#include "Particles.hlsli"

[maxvertexcount(1)]
void main(point ParticleVertex input[1], inout PointStream<ParticleVertex> stream)
{
	ParticleVertex output;

	// process particle
	output.position           = input[0].position;
	output.lifeSizeGrowWiggle = input[0].lifeSizeGrowWiggle;
	output.velocityFriction   = input[0].velocityFriction;
	output.color              = input[0].color;

	stream.Append(output);
}