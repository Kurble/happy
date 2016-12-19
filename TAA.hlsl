struct VSOut
{
	float4 pos : SV_Position;
	float2 tex : TexCoord;
};

cbuffer CBufferTAA : register(b2)
{
	float4x4 viewInverse;
	float4x4 projectionInverse;
	float4x4 viewHistory;
	float4x4 projectionHistory;
	float blendFactor;
	float texelWidth;
	float texelHeight;
};

#include "Utils.hlsli"

SamplerState g_ScreenSampler       : register(s0);
SamplerState g_TextureSampler      : register(s1);
Texture2D<float4> g_SceneBuffer    : register(t0);
Texture2D<float4> g_HistoryBuffer  : register(t1);
Texture2D<float>  g_DepthBuffer    : register(t2);
Texture2D<float2> g_VelocityBuffer : register(t3);

float4 main(VSOut input) : SV_TARGET
{
	//=========================================================
	// Find the location where the history pixel is
	//=========================================================
	float3 currentPosition = calcPosition(input.tex, g_DepthBuffer.Sample(g_ScreenSampler, input.tex));
	float4 historyPosition = mul(viewHistory, float4(currentPosition, 1));
	historyPosition = mul(projectionHistory, historyPosition);

	// todo: take motion into account

	//=========================================================
	// Sample the current neighbourhood
	//=========================================================
	const float4 nbh[9] = 
	{
		convertToYCoCg(g_SceneBuffer.Sample(g_ScreenSampler, float2(input.tex.x - texelWidth, input.tex.y - texelHeight))),
		convertToYCoCg(g_SceneBuffer.Sample(g_ScreenSampler, float2(input.tex.x - texelWidth, input.tex.y              ))),
		convertToYCoCg(g_SceneBuffer.Sample(g_ScreenSampler, float2(input.tex.x - texelWidth, input.tex.y + texelHeight))),
		convertToYCoCg(g_SceneBuffer.Sample(g_ScreenSampler, float2(input.tex.x,              input.tex.y - texelHeight))),
		convertToYCoCg(g_SceneBuffer.Sample(g_ScreenSampler, float2(input.tex.x,              input.tex.y              ))),
		convertToYCoCg(g_SceneBuffer.Sample(g_ScreenSampler, float2(input.tex.x,              input.tex.y + texelHeight))),
		convertToYCoCg(g_SceneBuffer.Sample(g_ScreenSampler, float2(input.tex.x + texelWidth, input.tex.y - texelHeight))),
		convertToYCoCg(g_SceneBuffer.Sample(g_ScreenSampler, float2(input.tex.x + texelWidth, input.tex.y              ))),
		convertToYCoCg(g_SceneBuffer.Sample(g_ScreenSampler, float2(input.tex.x + texelWidth, input.tex.y + texelHeight))),
	};
	const float4 color = nbh[4];

	//=========================================================
	// Create an YCoCg min/max box to clip history to
	//=========================================================
	const float4 minimum = min(min(min(min(min(min(min(min(nbh[0], nbh[1]), nbh[2]), nbh[3]), nbh[4]), nbh[5]), nbh[6]), nbh[7]), nbh[8]);
	const float4 maximum = max(max(max(max(max(max(max(max(nbh[0], nbh[1]), nbh[2]), nbh[3]), nbh[4]), nbh[5]), nbh[6]), nbh[7]), nbh[8]);
	const float4 average = (nbh[0] + nbh[1] + nbh[2] + nbh[3] + nbh[4] + nbh[5] + nbh[6] + nbh[7] + nbh[8]) * .1111111111111111111111111f;

	//=========================================================
	// History clipping
	//=========================================================
	float4 history = convertToYCoCg(g_HistoryBuffer.Sample(g_ScreenSampler, (historyPosition.xy/historyPosition.w) * float2(0.5f, -0.5f) + 0.5f));
	
	const float3 origin = history.rgb - 0.5f*(minimum.rgb + maximum.rgb);
	const float3 direction = average.rgb - history.rgb;
	const float3 extents = maximum.rgb - 0.5f*(minimum.rgb + maximum.rgb);

	history = lerp(history, average, saturate(intersectAABB(origin, direction, extents)));

	//=========================================================
	// Calculate results
	//=========================================================
	float impulse = abs(color.x - history.x) / max(color.x, max(history.x, minimum.x));
	float factor = lerp(blendFactor * 0.8f, blendFactor * 2.0f, impulse*impulse);

	return convertToRGBA(lerp(history, color, blendFactor));
	//return convertToRGBA(color);
}