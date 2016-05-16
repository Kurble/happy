cbuffer CBufferScene : register(b0)
{
	float4x4 view;
	float4x4 projection;
	float4x4 viewInverse;
	float4x4 projectionInverse;
	float width;
	float height;
	unsigned int convolutionStages;
};

cbuffer CBufferObject : register(b1)
{
	float4x4 world;
	float2 animationChannelBlend;
	float  animationFrameBlend;
};