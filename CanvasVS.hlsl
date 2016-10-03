struct VSIn
{
	float4 pos : POSITION;
	float4 col : TEXCOORD0;
};

struct VSOut
{
	float4 pos : SV_POSITION;
	float4 col : TEXCOORD0;
};

VSOut main(VSIn input)
{
	VSOut output;
	output.pos = input.pos;
	output.col = input.col;
	return output;
}