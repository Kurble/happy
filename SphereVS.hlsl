#include "CBuffers.h"
struct VSOut
{
	float4 position : SV_Position;
	float2 screenPos : TEXCOORD0;
};

struct VSIn
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD0;
};

cbuffer CBufferPointLight : register(b1)
{
	float4 lightPosition;
	float4 lightColor;
	float lightSize;
	float lightFalloff;
}

VSOut main(VSIn input)
{
	VSOut output;

	output.position  = float4((input.position.xyz * lightSize) + lightPosition.xyz, 1);
	output.position  = mul(view, output.position);
	output.position  = mul(projection, output.position);
	output.screenPos = (output.position.xy / output.position.w)*float2(.5f, -.5f) + .5f;

	return output;
}