#include "Shared.hlsli"

struct VS_INPUT
{
	float4	Position	: POSITION;
	float4	Color		: COLOR;
	float3	TexCoord	: TEXCOORD;
	float4	Normal		: NORMAL;
	float4	Tangent		: TANGENT;

	// Instance
	uint	InstanceID		: SV_InstanceID;
	float4	InstanceWorld0	: INSTANCEWORLD0;
	float4	InstanceWorld1	: INSTANCEWORLD1;
	float4	InstanceWorld2	: INSTANCEWORLD2;
	float4	InstanceWorld3	: INSTANCEWORLD3;
};

struct VS_INPUT_ANIMATION
{
	float4	Position	: POSITION;
	float4	Color		: COLOR;
	float3	TexCoord	: TEXCOORD;
	float4	Normal		: NORMAL;
	float4	Tangent		: TANGENT;

	uint4	BoneIndex	: BLENDINDICES;
	float4	BoneWeight	: BLENDWEIGHT;

	uint	InstanceID	: SV_InstanceID;
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
	uint	InstanceID		: INSTANCEID;
};

#define HS_OUTPUT VS_OUTPUT
#define DS_OUTPUT HS_OUTPUT

struct HS_CONSTANT_DATA_OUTPUT
{
	float EdgeTessFactor[3]	: SV_TessFactor;
	float InsideTessFactor : SV_InsideTessFactor;
};