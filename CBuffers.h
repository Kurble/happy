cbuffer CBufferScene : register(b0)
{
	float4x4 view;
	float4x4 projection;
	float4x4 viewInverse;
	float4x4 projectionInverse;
	float width;
	float height;
	unsigned int convolutionStages;
	unsigned int aoEnabled;
};

cbuffer CBufferObject : register(b1)
{
	float4x4 world;
	float4x4 worldInverse;
	float alpha;
};

cbuffer CBufferSkin : register(b2)
{
	float4 animationBlend;
	float4 frameBlend;
	uint animationCount;
}