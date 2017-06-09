struct ParticleVertex
{
	float4 positionRotation   : POSITION;
	float4 lifeSizeGrowWiggle : TEXCOORD0;
	float4 velocityFriction   : TEXCOORD1;
	float4 gravitySpin        : TEXCOORD2;
	float4 color1             : TEXCOORD3;
	float4 color2             : TEXCOORD4;
	float4 color3             : TEXCOORD5;
	float4 color4             : TEXCOORD6;
	float4 stops              : TEXCOORD7;
	float4 texCoord           : TEXCOORD8;
};

#define PART_POSITION positionRotation.xyz
#define PART_ROTATION positionRotation.w
#define PART_LIFE lifeSizeGrowWiggle.x
#define PART_SIZE lifeSizeGrowWiggle.y
#define PART_GROW lifeSizeGrowWiggle.z
#define PART_WIGGLE lifeSizeGrowWiggle.w
#define PART_VELOCITY velocityFriction.xyz
#define PART_FRICTION velocityFriction.w
#define PART_GRAVITY gravitySpin.xyz
#define PART_SPIN gravitySpin.w
#define PART_COLOR1 color1
#define PART_COLOR2 color2
#define PART_COLOR3 color3
#define PART_COLOR4 color4
#define PART_STOPS stops
#define PART_TEXCOORD texCoord

struct ParticleRenderVertex
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
	float4 color    : TEXCOORD1;
};