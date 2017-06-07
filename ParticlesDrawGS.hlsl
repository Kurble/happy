#include "GBufferCommon.hlsli"
#include "Particles.hlsli"

[maxvertexcount(4)]
void main(point ParticleVertex input[1], inout TriangleStream<ParticleVertex> stream)
{
	ParticleVertex output;

	// fill in output
	output.position           = input[0].position;
	output.lifeSizeGrowWiggle = input[0].lifeSizeGrowWiggle;
	output.velocityFriction   = input[0].velocityFriction;
	output.color              = input[0].color;

	// draw particle
	output.position = mul(jitteredView, output.position + float4(-1, -1, 0, 0));
	output.position = mul(jitteredProjection, output.position);
	stream.Append(output);
	output.position = mul(jitteredView, output.position + float4(1, -1, 0, 0));
	output.position = mul(jitteredProjection, output.position);
	stream.Append(output);
	output.position = mul(jitteredView, output.position + float4(-1, 1, 0, 0));
	output.position = mul(jitteredProjection, output.position);
	stream.Append(output);
	output.position = mul(jitteredView, output.position + float4(1, 1, 0, 0));
	output.position = mul(jitteredProjection, output.position);
	stream.Append(output);
}