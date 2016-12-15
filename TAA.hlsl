struct VSOut
{
	float4 pos : SV_Position;
	float2 tex : TexCoord;
};

cbuffer CBufferScene : register(b0)
{
	float4x4 view;
	float4x4 projection;
	float4x4 viewInverse;
	float4x4 projectionInverse;
	float width;
	float height;
	unsigned int convolutionStages;
	unsigned int aoEnabled;
};

SamplerState g_ScreenSampler    : register(s0);
SamplerState g_TextureSampler   : register(s1);
Texture2D<float4> g_SceneBuffer : register(t0);
Texture2D<float>  g_DepthBuffer : register(t1);
Texture2D<float>  g_VisionMap   : register(t2);

float3 getPosition(float2 tex, float depth)
{
	float4 position = float4(
		tex.x * (2) - 1,
		tex.y * (-2) + 1,
		depth,
		1.0f
		);

	position = mul(projectionInverse, position);
	position = mul(viewInverse, position);

	return position.xyz / position.w;
}

float4 main(VSOut input) : SV_TARGET
{
	return float4(1, 1, 1, 1);
}