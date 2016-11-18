#include "GBufferCommon.h"

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

	output.position  = mul(world,           input.position);
	output.position  = mul(view,            output.position);
	output.position  = mul(projection,      output.position);
	output.normal    = normalize(mul((float3x3)world, input.normal));
	output.tangent   = normalize(mul((float3x3)world, input.tangent));
	output.binormal  = normalize(mul((float3x3)world, input.binormal));
	output.texcoord0 = input.texcoord;
	output.texcoord1 = input.texcoord;

	return output;
}