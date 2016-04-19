cbuffer Convolution : register(b0)
{
	float4x4 g_Side;
	float g_Exponent;

	float g_Size;
	float g_Start;
	float g_End;
	float g_Incr;
};

struct VSOut
{
	float4 position : SV_Position;
	float3 coord : NORMAL;
};

SamplerState g_TextureSampler : register(s0);

TextureCube<float4> g_Source : register(t0);

static const float3 x = float3(1.0, 0.0, 0.0);
static const float3 y = float3(0.0, 1.0, 0.0);
static const float3 z = float3(0.0, 0.0, 1.0);

static const float3x3 front  = float3x3(x, y,  z);
static const float3x3 back   = float3x3(x, y, -z);
static const float3x3 right  = float3x3(z, y,  x);
static const float3x3 left   = float3x3(z, y, -x);
static const float3x3 top    = float3x3(x, z,  y);
static const float3x3 bottom = float3x3(x, z, -y);

float4 getsample(float3x3 side, float3 eyedir, float3 base_ray)
{
	float3 ray = mul(base_ray, side);

	float lambert = max(0.0, pow(abs(dot(ray, eyedir)), g_Exponent));

	float4 hdr = g_Source.Sample(g_TextureSampler, ray);
	float3 col = hdr.rgb *(pow(abs(hdr.a), 1.1) * 1);

	return float4(col*lambert, lambert);
}

float4 main(VSOut input) : SV_Target
{
	float4 result = 0;
	float3 eyedir = normalize(input.coord), ray;

	for (float xi = g_Start; xi <= g_End; xi += g_Incr)
	{
		for (float yi = g_Start; yi <= g_End; yi += g_Incr)
		{
			ray = normalize(float3(xi, yi, 1.0f));

			result += getsample(front, eyedir, ray);
			result += getsample(back, eyedir, ray);
			result += getsample(top, eyedir, ray);
			result += getsample(bottom, eyedir, ray);
			result += getsample(left, eyedir, ray);
			result += getsample(right, eyedir, ray);
		}
	}
	result /= result.w;
	return float4(pow(abs(result.rgb), 1.0 / 2.2), 1.0);
}