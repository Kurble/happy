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
		float4x4 frame_a0 = lerp(pose0[bone], pose1[bone], animationFrameBlend);
		float4x4 frame_a1= lerp(pose2[bone], pose3[bone], animationFrameBlend);
		float4x4 animation = lerp(frame_a0, frame_a1, animationChannelBlend.x);

		return mul(bindpose[bone], animation);
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

	output.position  = mul(skinTransform,   input.position);
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