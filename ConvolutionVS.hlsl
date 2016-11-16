cbuffer Convolution : register(b0)
{
	float4x4 g_Side;
	float g_Roughness;

	float g_Size;
	float g_Start;
	float g_End;
	float g_Incr;
};

struct VSOut
{
	float4 position : SV_Position;
	float3 coord : TEXCOORD0;
};

VSOut main(float4 position : POSITION)
{
	float3 ray = normalize(float3(position.xy, 1));

	VSOut output;
	output.position = position;
	output.coord = mul((float3x3)g_Side, ray);
	return output;
}