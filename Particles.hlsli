
// TODO                                       [floats]
//-[ ] remove growth and size parameter:      -2
//-[ ] 2nd degree size: ax^2 + bx + c         +3
//-[ ] Brownian speed                         +1
//-[ ] Brownian friction                      +1
//-[ ] Brownian scale                         +1
//-[ ] Don't do wiggle                        -1
//-[ ] Max life counter (decrease is annoy)   +1
//----------------------------------------------- +
//                                            +4!!

struct ParticleVertex
{
	float4 attrPos : POSITION;
	float4 attr0   : TEXCOORD0;
	float4 attr1   : TEXCOORD1;
	float4 attr2   : TEXCOORD2;
	float4 attr3   : TEXCOORD3;
	float4 attr4   : TEXCOORD4;
	float4 attr5   : TEXCOORD5;
	float4 attr6   : TEXCOORD6;
	float4 attr7   : TEXCOORD7;
	float4 attr8   : TEXCOORD8;
	float4 tex   : TEXCOORD9;
};

#define PART_POSITION          attrPos.xyz
#define PART_ROTATION          attrPos.w
#define PART_SIZE1             attr0.x
#define PART_SIZE2             attr0.y
#define PART_SIZE3             attr0.z
#define PART_SIZE4             attr0.w
#define PART_VELOCITY          attr1.xyz
#define PART_FRICTION          attr1.w
#define PART_BROWNIAN          attr2.x
#define PART_BROWNIAN_FRICTION attr2.y
#define PART_BROWNIAN_SCALE    attr2.z
#define PART_LIFE              attr2.w
#define PART_GRAVITY           attr3.xyz
#define PART_SPIN              attr3.w
#define PART_COLOR1            attr4
#define PART_COLOR2            attr5
#define PART_COLOR3            attr6
#define PART_COLOR4            attr7
#define PART_STOPS             attr8
#define PART_TEXCOORD          tex

struct ParticleRenderVertex
{
	float4 position : SV_Position;
	float2 texCoord : TEXCOORD0;
	float4 color    : TEXCOORD1;
};