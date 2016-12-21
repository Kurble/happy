#include "GBufferCommon.hlsli"

cbuffer CBufferBindPose    : register(b3 ) { float4x4 bindpose[64]; };
cbuffer CBufferAnim0Frame0 : register(b4 ) { float4x4 pose0[64]; }
cbuffer CBufferAnim0Frame1 : register(b5 ) { float4x4 pose1[64]; }
cbuffer CBufferAnim1Frame0 : register(b6 ) { float4x4 pose2[64]; }
cbuffer CBufferAnim1Frame1 : register(b7 ) { float4x4 pose3[64]; }
cbuffer CBufferAnim2Frame0 : register(b8 ) { float4x4 pose4[64]; }
cbuffer CBufferAnim2Frame1 : register(b9 ) { float4x4 pose5[64]; }
cbuffer CBufferAnim3Frame0 : register(b10) { float4x4 pose6[64]; }
cbuffer CBufferAnim3Frame1 : register(b11) { float4x4 pose7[64]; }

struct VSIn
{
	float4 position : POSITION;
	float3 normal :   TEXCOORD0;
	float3 tangent :  TEXCOORD1;
	float3 binormal : TEXCOORD2;
	float2 texcoord : TEXCOORD3;
	uint4  indices :  TEXCOORD4;
	float4 weights :  TEXCOORD5;
};

float4x4 resolveBoneMatrix(uint bone)
{
	if (bone >= 63)
	{
		return 0;
	}
	else
	{
		float4x4 animation                 = animationBlend.x * lerp(pose0[bone], pose1[bone], frameBlend.x);
		if (animationCount > 1) animation += animationBlend.y * lerp(pose2[bone], pose3[bone], frameBlend.y);
		if (animationCount > 2) animation += animationBlend.z * lerp(pose4[bone], pose5[bone], frameBlend.z);
		if (animationCount > 3) animation += animationBlend.w * lerp(pose6[bone], pose7[bone], frameBlend.w);
		return mul(animation, bindpose[bone]);
	}
}

float3 transformNormal(float3 normal, float4x4 skinTransform)
{
	float4 normal4 = float4(normal, 0.0f);
	//normal4 = mul(skinTransform, normal4);
	normal4 = mul(world,         normal4);
	return normal4.xyz;
}

VSOut main(VSIn input)
{
	VSOut output;

	float4x4 skinTransform =
		input.weights.x * resolveBoneMatrix(input.indices.x) +
		input.weights.y * resolveBoneMatrix(input.indices.y) +
		input.weights.z * resolveBoneMatrix(input.indices.z) +
		input.weights.w * resolveBoneMatrix(input.indices.w);

	float3x3 normalTransform = mul((float3x3)world, (float3x3)skinTransform);

	output.position =  mul(skinTransform,      input.position);
	output.position  = mul(world,              output.position);
	output.position  = mul(jitteredView,       output.position);
	output.position  = mul(jitteredProjection, output.position);
	output.normal    = normalize(mul(normalTransform, input.normal));
	output.tangent   = normalize(mul(normalTransform, input.tangent));
	output.binormal  = normalize(mul(normalTransform, input.binormal));
	output.texcoord0 = input.texcoord;
	output.texcoord1 = input.texcoord;
	output.velocity = float4(0, 0, 0, 0);

	return output;
}