struct VSOut
{
	float4 pos : SV_Position;
	float2 tex : TexCoord;
};

SamplerState g_ScreenSampler       : register(s0);
SamplerState g_TextureSampler      : register(s1);
Texture2D<float4> g_SceneBuffer    : register(t0);

float4 main(VSOut input) : SV_TARGET
{
	return g_SceneBuffer.Sample(g_ScreenSampler, input.tex);
}