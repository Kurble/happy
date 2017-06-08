#include "GBufferCommon.hlsli"
#include "Particles.hlsli"

[maxvertexcount(4)]
void main(point ParticleVertex input[1], inout TriangleStream<ParticleRenderVertex> stream)
{
	ParticleRenderVertex output;

	// fill in output
	output.color              = input[0].color;
	output.texCoord           = input[0].texCoord;

	float4 position = input[0].position;
	position = mul(jitteredView, input[0].position);
	position = mul(jitteredProjection, position);

	float w = (0.5f * input[0].lifeSizeGrowWiggle.y * jitteredProjection[0][0]);
	float h = (0.5f * input[0].lifeSizeGrowWiggle.y * jitteredProjection[1][1]);
	
	output.position = position + float4(-w, -h, 0, 0);
	stream.Append(output);
	
	output.position = position + float4(-w, +h, 0, 0);
	stream.Append(output);
	
	output.position = position + float4(+w, -h, 0, 0);
	stream.Append(output);

	output.position = position + float4(+w, +h, 0, 0);
	stream.Append(output);
}