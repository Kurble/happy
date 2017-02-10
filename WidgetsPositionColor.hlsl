#include "CBuffers.hlsli"

struct VSOut
{
	float4 position : SV_Position;
	float4 color    : TEXCOORD0;
};

struct VSIn
{
	float4 position : POSITION;
	float4 color    : TEXCOORD0;
};

VSOut main(VSIn input)
{
	VSOut output;

	output.position = mul(jitteredView, input.position);
	output.position = mul(jitteredProjection, output.position);
	output.color = input.color;

	return output;
}