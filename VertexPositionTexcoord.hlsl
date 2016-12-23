#include "GBufferCommon.hlsli"

struct VSIn
{
	float4 position : POSITION;
	float2 texcoord : TEXCOORD0;
};

VSOut main(VSIn input)
{
	VSOut output;

	output.position         = mul(currentWorld,       input.position);
	output.position         = mul(jitteredView,       output.position);
	output.position         = mul(jitteredProjection, output.position);
	output.previousPosition = mul(previousWorld,      input.position);
	output.previousPosition = mul(previousView,       output.previousPosition);
	output.previousPosition = mul(previousProjection, output.previousPosition);
	output.currentPosition  = output.position;
	output.normal           = 0;
	output.tangent          = 0;
	output.binormal         = 0;
	output.texcoord0        = input.texcoord;
	output.texcoord1        = input.texcoord;

	return output;
}