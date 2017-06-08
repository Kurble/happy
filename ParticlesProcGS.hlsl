#include "GBufferCommon.hlsli"
#include "Particles.hlsli"

[maxvertexcount(1)]
void main(point ParticleVertex input[1], inout PointStream<ParticleVertex> stream)
{
	ParticleVertex output;

	float velocity = length(input[0].PART_VELOCITY);
	float damping = max(0, velocity - input[0].PART_FRICTION * timestep) / velocity;

	// process particle
	output.PART_POSITION = input[0].PART_POSITION + float4(input[0].PART_VELOCITY * timestep, 0.0f);
	output.PART_LIFE     = input[0].PART_LIFE - timestep;
	output.PART_SIZE     = input[0].PART_SIZE + input[0].PART_GROW * timestep;
	output.PART_GROW     = input[0].PART_GROW;
	output.PART_WIGGLE   = input[0].PART_WIGGLE;
	output.PART_VELOCITY = input[0].PART_VELOCITY * damping;
	output.PART_FRICTION = input[0].PART_FRICTION;
	output.PART_COLOR    = input[0].PART_COLOR;
	output.PART_TEXCOORD = input[0].PART_TEXCOORD;

	if (output.PART_LIFE > 0)
	{
		stream.Append(output);
	}
}