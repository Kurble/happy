struct VSOut
{
	float4 pos : SV_POSITION;
	float4 col : TEXCOORD0;
};

float4 main(VSOut input) : SV_TARGET
{
	return input.col;
}