float chiGGX(float v)
{
	return v > 0 ? 1 : 0;
}

float GGX_Distribution(float3 n, float3 h, float alpha)
{
	float NoH = dot(n, h);
	float alpha2 = alpha * alpha;
	float NoH2 = NoH * NoH;
	float den = NoH2 * alpha2 + (1 - NoH2);
	return (chiGGX(NoH) * alpha2) / (PI * den * den);
}