struct ParticleVertex
{
	float4 position           : POSITION;
	float4 lifeSizeGrowWiggle : TEXCOORD0;
	float4 velocityFriction   : TEXCOORD1;
	float4 color              : TEXCOORD2;
	float4 texCoord           : TEXCOORD3;
};

#define PART_POSITION position
#define PART_LIFE lifeSizeGrowWiggle.x
#define PART_SIZE lifeSizeGrowWiggle.y
#define PART_GROW lifeSizeGrowWiggle.z
#define PART_WIGGLE lifeSizeGrowWiggle.w
#define PART_VELOCITY velocityFriction.xyz
#define PART_FRICTION velocityFriction.w
#define PART_COLOR color
#define PART_TEXCOORD texCoord

struct ParticleRenderVertex
{
	float4 position : SV_Position;
	float4 texCoord : TEXCOORD0;
	float4 color    : TEXCOORD1;
};