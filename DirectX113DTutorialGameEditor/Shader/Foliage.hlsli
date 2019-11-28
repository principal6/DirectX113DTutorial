#include "Shared.hlsli"

struct VS_FOLIAGE_INPUT
{
	float4 Position		: POSITION;
	float4 Color		: COLOR;
	float3 TexCoord		: TEXCOORD;
	float4 Normal		: NORMAL;
	float4 Tangent		: TANGENT;

	// Instance
	float4 InstanceWorld0	: INSTANCEWORLD0;
	float4 InstanceWorld1	: INSTANCEWORLD1;
	float4 InstanceWorld2	: INSTANCEWORLD2;
	float4 InstanceWorld3	: INSTANCEWORLD3;
};

struct VS_FOLIAGE_OUTPUT
{
	float4	Position		: SV_POSITION;
	float4	WorldPosition	: POSITION;
	float3	TexCoord		: TEXCOORD;
	float4	WorldNormal		: NORMAL;
};

static float2 GetHeightMapUVFromPosition(float2 TerrainSize, float2 XZ)
{
	return float2((XZ.x + (TerrainSize.x / 2.0f)) / TerrainSize.x, (-XZ.y + (TerrainSize.y / 2.0f)) / TerrainSize.y);
}