#include "GBufferCommon.hlsli"

struct VSIn
{
	float4 position : POSITION;
	float3 normal   : TEXCOORD0;
	float2 texcoord : TEXCOORD1;
};

VSOut main(VSIn input)
{
	VSOut output;

	output.position         = mul(currentWorld,       input.position);
	output.worldPosition    = output.position.xyz;
	output.position         = mul(jitteredView,       output.position);
	output.position         = mul(jitteredProjection, output.position);
	output.previousPosition = mul(previousWorld,      input.position);
	output.previousPosition = mul(previousView,       output.previousPosition);
	output.previousPosition = mul(previousProjection, output.previousPosition);
	output.currentPosition  = output.position;
	output.normal           = normalize(mul((float3x3)currentWorld, input.normal));
	output.tangent          = 0;
	output.binormal         = 0;
	output.texcoord0        = input.texcoord;
	output.texcoord1        = input.texcoord;

	return output;
}