#include "GBufferCommon.h"

cbuffer CBufferBindPose    : register(b2) { float4x4 bindpose[126]; };
cbuffer CBufferAnim0Frame0 : register(b3) { float4x4 pose0[126]; }
cbuffer CBufferAnim0Frame1 : register(b4) { float4x4 pose1[126]; }
cbuffer CBufferAnim1Frame0 : register(b5) { float4x4 pose2[126]; }
cbuffer CBufferAnim1Frame1 : register(b6) { float4x4 pose3[126]; }

struct VSIn
{
	float4 position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT0;
	float3 binormal : TANGENT1;
	float2 texcoord : TEXCOORD;
	uint4  indices : TEXCOORD1;
	float4 weights : TEXCOORD2;
};

float4x4 resolveBoneMatrix(uint bone)
{
	if (bone >= 126)
	{
		return 0;
	}
	else
	{
		float4x4 animation = lerp(pose0[bone], pose1[bone], frameBlend.x);
		
		return mul(animation, bindpose[bone]);
	}
}

VSOut main(VSIn input)
{
	VSOut output;

	float4x4 skinTransform =
		input.weights.x * resolveBoneMatrix(input.indices.x) +
		input.weights.y * resolveBoneMatrix(input.indices.y) +
		input.weights.z * resolveBoneMatrix(input.indices.z) +
		input.weights.w * resolveBoneMatrix(input.indices.w);

	float4 huh = input.position;
	huh.x = huh.x * -1;
	huh.y = huh.y * -1;

	output.position  = mul(skinTransform,   huh);
	output.position  = mul(world,           output.position);
	output.position  = mul(view,            output.position);
	output.position  = mul(projection,      output.position);
	output.normal    = mul((float3x3)world, input.normal);
	output.tangent   = mul((float3x3)world, input.tangent);
	output.binormal  = mul((float3x3)world, input.binormal);
	output.texcoord0 = input.texcoord;
	output.texcoord1 = input.texcoord;

	return output;
}