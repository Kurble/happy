#include "GBufferCommon.hlsli"
#include "Particles.hlsli"

[maxvertexcount(1)]
void main(point ParticleVertex input[1], inout PointStream<ParticleVertex> stream)
{
	if (input[0].PART_LIFE >= 0)
	{
		ParticleVertex output = input[0];

		float velocity = length(input[0].PART_VELOCITY);
		float damping = velocity > 0 ? max(0, velocity - input[0].PART_FRICTION * timestep) / velocity : 1;

		// process particle
		output.PART_POSITION = input[0].PART_POSITION + input[0].PART_VELOCITY * timestep;
		output.PART_ROTATION = input[0].PART_ROTATION + input[0].PART_SPIN * timestep;
		output.PART_LIFE     = input[0].PART_LIFE - timestep;
		output.PART_VELOCITY = input[0].PART_VELOCITY * damping + input[0].PART_GRAVITY.xyz * timestep;
		output.PART_BROWNIAN = max(0, input[0].PART_BROWNIAN - input[0].PART_BROWNIAN_FRICTION * timestep);
		
		stream.Append(output);
	}
}