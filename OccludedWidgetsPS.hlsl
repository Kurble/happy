struct VSOut
{
	float4 position : SV_Position;
	float4 color    : TEXCOORD0;
};

float4 main(VSOut input) : SV_TARGET
{
	uint coord = (((uint)input.position.x) % 2) * 2 + (((uint)input.position.y) % 2);
	if (coord > 0)
	{
		discard;
	}
	return input.color;
}