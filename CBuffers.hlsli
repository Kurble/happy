cbuffer CBufferScene : register(b0)
{
	float4x4 jitteredView;
	float4x4 jitteredProjection;
	float4x4 inverseView;
	float4x4 inverseProjection;
	float4x4 currentView;
	float4x4 currentProjection;
	float4x4 previousView;
	float4x4 previousProjection;
	float width;
	float height;
	unsigned int convolutionStages;
	unsigned int aoEnabled;
};

cbuffer CBufferObject : register(b1)
{
	float4x4 world;
	float4x4 inverseWorld;
	float4x4 previousWorld;
	float alpha;
};

cbuffer CBufferSkin : register(b2)
{
	float4 animationBlend;
	float4 frameBlend;
	uint animationCount;
}