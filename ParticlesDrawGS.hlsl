#include "GBufferCommon.hlsli"
#include "Particles.hlsli"

float4 rotate2D(float rot, float4 vec)
{
	return float4(vec.x*cos(rot) - vec.y*sin(rot), vec.x*sin(rot) + vec.y*cos(rot), vec.z, vec.w);
}

[maxvertexcount(4)] // will be 96 in the future: extrude mechanic
void main(point ParticleVertex input[1], inout TriangleStream<ParticleRenderVertex> stream)
{
	ParticleRenderVertex output;

	float3 blend = float3(saturate((input[0].PART_STOPS.x - input[0].PART_LIFE) / (input[0].PART_STOPS.x - input[0].PART_STOPS.y)),
		                  saturate((input[0].PART_STOPS.y - input[0].PART_LIFE) / (input[0].PART_STOPS.y - input[0].PART_STOPS.z)),
		                  saturate((input[0].PART_STOPS.z - input[0].PART_LIFE) / (input[0].PART_STOPS.z - input[0].PART_STOPS.w)));

	output.color = input[0].PART_COLOR1 + input[0].PART_COLOR2 * blend.x + input[0].PART_COLOR3 * blend.y + input[0].PART_COLOR4 * blend.z;
	float size = input[0].PART_SIZE1 + input[0].PART_SIZE2 * blend.x + input[0].PART_SIZE3 * blend.y + input[0].PART_SIZE4 * blend.z;
	
	float4 wPos = float4(input[0].PART_POSITION, 1.0f);
	float4 vPos = mul(jitteredView, wPos);
	float4 pPos = mul(jitteredProjection, vPos);
	
	if (size > 0)
	{
		float4 sza = mul(currentProjection, float4(0, 0, vPos.z, 1.0f));
		float4 szb = mul(currentProjection, float4(size, size, vPos.z, 1.0f));
		float w = szb.x-sza.x;
		float h = szb.y-sza.y;

		output.position = pPos + rotate2D(input[0].PART_ROTATION, float4(-1, -1, 0, 0)) * float4(w, h, 0, 0);
		output.texCoord = input[0].tex.xy;
		stream.Append(output);

		output.position = pPos + rotate2D(input[0].PART_ROTATION, float4(-1, +1, 0, 0)) * float4(w, h, 0, 0);
		output.texCoord = input[0].tex.xy + float2(0, input[0].tex.w);
		stream.Append(output);

		output.position = pPos + rotate2D(input[0].PART_ROTATION, float4(+1, -1, 0, 0)) * float4(w, h, 0, 0);
		output.texCoord = input[0].tex.xy + float2(input[0].tex.z, 0);
		stream.Append(output);

		output.position = pPos + rotate2D(input[0].PART_ROTATION, float4(+1, +1, 0, 0)) * float4(w, h, 0, 0);
		output.texCoord = input[0].tex.xy + float2(input[0].tex.z, input[0].tex.w);
		stream.Append(output);
	}
}