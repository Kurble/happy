
// vs/gs OUT gs IN ps IN
struct ParticleVertex
{
	float4 position           : SV_Position;
	float4 lifeSizeGrowWiggle : TEXCOORD0;
	float4 velocityFriction   : TEXCOORD1;
	float4 color              : TEXCOORD2;
};

// vs IN
struct EmittedParticleVertex
{
	float4 position           : POSITION;
	float4 lifeSizeGrowWiggle : TEXCOORD0;
	float4 velocityFriction   : TEXCOORD1;
	float4 color              : TEXCOORD2;
};