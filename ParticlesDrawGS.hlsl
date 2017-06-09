#include "GBufferCommon.hlsli"
#include "Particles.hlsli"

float4 rotate2D(float rot, float4 vec)
{
	return float4(vec.x*cos(rot) - vec.y*sin(rot), vec.x*sin(rot) + vec.y*cos(rot), vec.z, vec.w);
}

[maxvertexcount(4)]
void main(point ParticleVertex input[1], inout TriangleStream<ParticleRenderVertex> stream)
{
	ParticleRenderVertex output;

	output.color = 
		input[0].PART_COLOR1 + 
		input[0].PART_COLOR2 * saturate((input[0].PART_STOPS.x - input[0].PART_LIFE) / (input[0].PART_STOPS.x - input[0].PART_STOPS.y)) +
		input[0].PART_COLOR3 * saturate((input[0].PART_STOPS.y - input[0].PART_LIFE) / (input[0].PART_STOPS.y - input[0].PART_STOPS.z)) +
		input[0].PART_COLOR4 * saturate((input[0].PART_STOPS.z - input[0].PART_LIFE) / (input[0].PART_STOPS.z - input[0].PART_STOPS.w));
	
	float4 position = float4(input[0].PART_POSITION, 1.0f);
	position = mul(jitteredView, position);
	position = mul(jitteredProjection, position);

	float w = (0.5f * input[0].lifeSizeGrowWiggle.y * jitteredProjection[0][0]);
	float h = (0.5f * input[0].lifeSizeGrowWiggle.y * jitteredProjection[1][1]);
	
	output.position = position + rotate2D(input[0].PART_ROTATION, float4(-1, -1, 0, 0)) * float4(w, h, 0, 0);
	output.texCoord = input[0].texCoord.xy;
	stream.Append(output);
	
	output.position = position + rotate2D(input[0].PART_ROTATION, float4(-1, +1, 0, 0)) * float4(w, h, 0, 0);
	output.texCoord = input[0].texCoord.xy + float2(0, input[0].texCoord.w);
	stream.Append(output);
	
	output.position = position + rotate2D(input[0].PART_ROTATION, float4(+1, -1, 0, 0)) * float4(w, h, 0, 0);
	output.texCoord = input[0].texCoord.xy + float2(input[0].texCoord.z, 0);
	stream.Append(output);

	output.position = position + rotate2D(input[0].PART_ROTATION, float4(+1, +1, 0, 0)) * float4(w, h, 0, 0);
	output.texCoord = input[0].texCoord.xy + float2(input[0].texCoord.z, input[0].texCoord.w);
	stream.Append(output);
}