float3 calcPosition(float2 tex, float depth)
{
	float4 position = float4(tex.x * (2) - 1, tex.y * (-2) + 1, depth, 1.0f);

	position = mul(projectionInverse, position);
	position = mul(viewInverse, position);

	return position.xyz / position.w;
}

float calcLinearDepth(float2 tex, float depth)
{
	float4 position = float4(tex.x * (2) - 1, tex.y * (-2) + 1, depth, 1.0f);

	position = mul(projectionInverse, position);

	return position.z / position.w;
}

float4 convertToYCoCg(float4 rgba)
{
	return float4(
		dot(rgba.rgb, float3( 0.25f, 0.50f,  0.25f)),
		dot(rgba.rgb, float3( 0.50f, 0.00f, -0.50f)),
		dot(rgba.rgb, float3(-0.25f, 0.50f, -0.25f)),
		rgba.a);
}

float4 convertToRGBA(float4 yCoCg)
{
	return float4(
		yCoCg.x + yCoCg.y - yCoCg.z,
		yCoCg.x + yCoCg.z,
		yCoCg.x - yCoCg.y - yCoCg.z,
		yCoCg.a);
}

float intersectAABB(float3 origin, float3 direction, float3 extents)
{
	float3 reciprocal = rcp(direction);
	float3 minimum = ( extents - origin) * reciprocal;
	float3 maximum = (-extents - origin) * reciprocal;
	
	return max(max(min(minimum.x, maximum.x), min(minimum.y, maximum.y)), min(minimum.z, maximum.z));
}