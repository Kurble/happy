struct VSOut
{
	float4 pos : SV_POSITION;
	float4 col : TEXCOORD0;
};

SamplerState g_TextureSampler : register(s0);
Texture2D<float4> g_TextureMap : register(t0);

float4 main(VSOut input) : SV_TARGET
{
	return g_TextureMap.Sample(g_TextureSampler, input.col.xy);
}