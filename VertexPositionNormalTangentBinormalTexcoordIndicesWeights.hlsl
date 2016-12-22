#include "GBufferCommon.hlsli"

cbuffer CBufferBindPose         : register(b3 ) { float4x4 bindpose[64]; };
cbuffer CBufferTime0Anim0Frame0 : register(b4 ) { float4x4 previousPose0_0[64]; }
cbuffer CBufferTime0Anim0Frame1 : register(b5 ) { float4x4 previousPose0_1[64]; }
cbuffer CBufferTime0Anim1Frame0 : register(b6 ) { float4x4 previousPose1_0[64]; }
cbuffer CBufferTime0Anim1Frame1 : register(b7 ) { float4x4 previousPose1_1[64]; }
cbuffer CBufferTime1Anim0Frame0 : register(b8 ) { float4x4 currentPose0_0[64]; }
cbuffer CBufferTime1Anim0Frame1 : register(b9 ) { float4x4 currentPose0_1[64]; }
cbuffer CBufferTime1Anim1Frame0 : register(b10) { float4x4 currentPose1_0[64]; }
cbuffer CBufferTime1Anim1Frame1 : register(b11) { float4x4 currentPose1_1[64]; }

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

float4x4 resolvePreviousBone(uint bone)
{
	if (bone >= 63)
	{
		return 0;
	}
	else
	{
		float4x4 animation                  = previousAnimationBlend.x * lerp(previousPose0_0[bone], previousPose0_1[bone], previousFrameBlend.x);
		if (animationCount == 2) animation += previousAnimationBlend.y * lerp(previousPose1_0[bone], previousPose1_1[bone], previousFrameBlend.y);
		return mul(animation, bindpose[bone]);
	}
}

float4x4 resolveCurrentBone(uint bone)
{
	if (bone >= 63)
	{
		return 0;
	}
	else
	{
		float4x4 animation                  = currentAnimationBlend.x * lerp(currentPose0_0[bone], currentPose0_1[bone], currentFrameBlend.x);
		if (animationCount == 2) animation += currentAnimationBlend.y * lerp(currentPose1_0[bone], currentPose1_1[bone], currentFrameBlend.y);
		return mul(animation, bindpose[bone]);
	}
}

VSOut main(VSIn input)
{
	VSOut output;

	float4x4 previousSkinTransform =
		input.weights.x * resolvePreviousBone(input.indices.x) +
		input.weights.y * resolvePreviousBone(input.indices.y) +
		input.weights.z * resolvePreviousBone(input.indices.z) +
		input.weights.w * resolvePreviousBone(input.indices.w);

	float4 previousVSPosition = mul(previousSkinTransform, input.position);
	previousVSPosition        = mul(previousWorld, previousVSPosition);
	previousVSPosition        = mul(previousView, previousVSPosition);

	float4x4 currentSkinTransform =
		input.weights.x * resolveCurrentBone(input.indices.x) +
		input.weights.y * resolveCurrentBone(input.indices.y) +
		input.weights.z * resolveCurrentBone(input.indices.z) +
		input.weights.w * resolveCurrentBone(input.indices.w);

	float4 currentVSPosition  = mul(currentSkinTransform, input.position);
	currentVSPosition         = mul(currentWorld, currentVSPosition);
	currentVSPosition         = mul(currentView, currentVSPosition);

	float3x3 normalTransform = mul((float3x3)currentWorld, (float3x3)currentSkinTransform);

	output.position         = mul(jitteredProjection, currentVSPosition);
	output.previousPosition = mul(previousProjection, previousVSPosition);
	output.normal           = normalize(mul(normalTransform, input.normal));
	output.tangent          = normalize(mul(normalTransform, input.tangent));
	output.binormal         = normalize(mul(normalTransform, input.binormal));
	output.texcoord0        = input.texcoord;
	output.texcoord1        = input.texcoord;

	return output;
}