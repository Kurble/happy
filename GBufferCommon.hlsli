#include "CBuffers.hlsli"

struct VSOut
{
	float4 position   : SV_Position;
	float3 normal     : TEXCOORD0;
	float3 tangent    : TEXCOORD1;
	float3 binormal   : TEXCOORD2;
	float2 texcoord0  : TEXCOORD3;
	float2 texcoord1  : TEXCOORD4;
	float4 currentPosition : TEXCOORD5;
	float4 previousPosition : TEXCOORD6; 
};