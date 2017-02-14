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

	output.position = mul(currentView, input.position);
	output.position = mul(currentProjection, output.position);
	output.position.z -= 0.01f;
	output.color = input.color;

	return output;
}