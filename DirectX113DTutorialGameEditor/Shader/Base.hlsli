#include "Shared.hlsli"

//#define DEBUG_SHADER

struct VS_INPUT
{
	float4	Position	: POSITION;
	float4	Color		: COLOR;
	float3	TexCoord	: TEXCOORD;
	float4	Normal		: NORMAL;
	float4	Tangent		: TANGENT;

// Instance
#ifndef DEBUG_SHADER
	uint	InstanceID		: SV_InstanceID;
#endif

// Instance
	float4	InstanceWorld0	: INSTANCE_WORLD0;
	float4	InstanceWorld1	: INSTANCE_WORLD1;
	float4	InstanceWorld2	: INSTANCE_WORLD2;
	float4	InstanceWorld3	: INSTANCE_WORLD3;
	float	IsHighlighted	: IS_HIGHLIGHTED;
};

struct VS_INPUT_ANIMATION
{
	float4	Position	: POSITION;
	float4	Color		: COLOR;
	float3	TexCoord	: TEXCOORD;
	float4	Normal		: NORMAL;
	float4	Tangent		: TANGENT;

	uint4	BoneIndex	: BLEND_INDICES;
	float4	BoneWeight	: BLEND_WEIGHT;

// Instance
#ifndef DEBUG_SHADER
	uint	InstanceID	: SV_InstanceID;
#endif

// Instance
	float4	InstanceWorld0	: INSTANCE_WORLD0;
	float4	InstanceWorld1	: INSTANCE_WORLD1;
	float4	InstanceWorld2	: INSTANCE_WORLD2;
	float4	InstanceWorld3	: INSTANCE_WORLD3;
	float	IsHighlighted	: IS_HIGHLIGHTED;
	float	AnimTick		: ANIM_TICK;
	uint	CurrAnimID		: CURR_ANIM_ID;
};

struct VS_OUTPUT
{
	float4	Position		: SV_POSITION;
	float4	WorldPosition	: POSITION;
	float4	Color			: COLOR;
	float3	TexCoord		: TEXCOORD;
	float4	WorldNormal		: NORMAL;
	float4	WorldTangent	: TANGENT;
	float4	WorldBitangent	: BITANGENT;
	int		bUseVertexColor : BOOL;

// Instance
#ifndef DEBUG_SHADER
	uint	InstanceID		: INSTANCEID;
#endif

// Instance
	float	IsHighlighted	: IS_HIGHLIGHTED;
};

#define HS_OUTPUT VS_OUTPUT
#define DS_OUTPUT HS_OUTPUT

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]	: SV_TessFactor;
	float InsideTessFactor : SV_InsideTessFactor;
};