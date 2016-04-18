#include "RendererCommon.h"

struct VSIn
{
	float4 pos : POSITION;
	float2 tex : TEXCOORD;
};

VSOut main(VSIn input)
{
	VSOut output;
	output.pos = input.pos;
	output.tex = input.tex;
	return output;
}