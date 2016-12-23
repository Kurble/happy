#include "GBufferCommon.hlsli"

struct VSIn
{
	float4 position : POSITION;
	float3 normal   : TEXCOORD0;
	float3 tangent  : TEXCOORD1;
	float3 binormal : TEXCOORD2;
	float2 texcoord : TEXCOORD3;
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
	output.normal           = normalize(mul((float3x3)currentWorld, input.normal));
	output.tangent          = normalize(mul((float3x3)currentWorld, input.tangent));
	output.binormal         = normalize(mul((float3x3)currentWorld, input.binormal));
	output.texcoord0        = input.texcoord;
	output.texcoord1        = input.texcoord;

	return output;
}