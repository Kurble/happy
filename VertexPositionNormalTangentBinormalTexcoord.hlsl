#include "GBufferCommon.h"

struct VSIn
{
	float4 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT0;
	float3 binormal : TANGENT1;
	float2 texcoord : TEXCOORD;
};

VSOut main(VSIn input)
{
	VSOut output;

	output.position  = mul(world,      input.position);
	output.position  = mul(view,       output.position);
	output.position  = mul(projection, output.position);
	output.normal    = mul((float3x3)world, input.normal);
	output.tangent   = mul((float3x3)world, input.tangent);
	output.binormal  = mul((float3x3)world, input.binormal);
	output.texcoord0 = input.texcoord;
	output.texcoord1 = input.texcoord;

	return output;
}