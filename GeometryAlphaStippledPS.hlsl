#include "GBufferCommon.hlsli"

struct GBufferOut
{
	float4 albedoRoughness  : SV_Target0;
	float4 normalMetallic : SV_Target1;
};

SamplerState g_TextureSampler : register(s0);

Texture2D<float4> g_AlbedoRoughnessMap : register(t0);
Texture2D<float4> g_NormalMetallicMap  : register(t1);

static const float _f = 65.0f;
static const float stipple[] =
{
	 1.0f/_f,  49.0f/_f,  13.0f/_f,  61.0f/_f,   4.0f/_f,  52.0f/_f,  16.0f/_f,  64.0f/_f,
	33.0f/_f,  17.0f/_f,  45.0f/_f,  29.0f/_f,  36.0f/_f,  20.0f/_f,  48.0f/_f,  32.0f/_f,
	 9.0f/_f,  57.0f/_f,   5.0f/_f,  53.0f/_f,  12.0f/_f,  60.0f/_f,   8.0f/_f,  56.0f/_f,
	41.0f/_f,  25.0f/_f,  37.0f/_f,  21.0f/_f,  44.0f/_f,  28.0f/_f,  40.0f/_f,  24.0f/_f,
	 3.0f/_f,  51.0f/_f,  15.0f/_f,  63.0f/_f,   2.0f/_f,  50.0f/_f,  14.0f/_f,  62.0f/_f,
	35.0f/_f,  19.0f/_f,  47.0f/_f,  31.0f/_f,  34.0f/_f,  18.0f/_f,  46.0f/_f,  30.0f/_f,
	11.0f/_f,  59.0f/_f,   7.0f/_f,  55.0f/_f,  10.0f/_f,  58.0f/_f,   6.0f/_f,  54.0f/_f,
	43.0f/_f,  27.0f/_f,  39.0f/_f,  23.0f/_f,  42.0f/_f,  26.0f/_f,  38.0f/_f,  22.0f/_f,
};

GBufferOut main(VSOut input)
{
	GBufferOut output;

	float3x3 normalMatrix = float3x3(
		input.tangent,
		input.binormal,
		input.normal
	);

	uint coord = (((uint)input.position.x) % 8) * 8 + ((uint)input.position.y) % 8;
	if (alpha < stipple[coord])
	{
		discard;
	}

	output.albedoRoughness = g_AlbedoRoughnessMap.Sample(g_TextureSampler, input.texcoord0);
	float4 normalMetallic = g_NormalMetallicMap.Sample(g_TextureSampler, input.texcoord0);
	float3 normal = normalize(mul(2.0f*normalMetallic.xyz-1.0f, normalMatrix));
	float  metallic = normalMetallic.w;
	output.normalMetallic = float4(normal.x*0.5f+0.5f, normal.y*0.5f + 0.5f, normal.z*0.5f + 0.5f, metallic);

	return output;
}