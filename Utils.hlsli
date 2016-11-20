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