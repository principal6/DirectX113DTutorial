#include "Terrain.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 World;
	float4x4 ViewProjection;
}

cbuffer cbTerrain : register(b1)
{
	float TerrainSizeX;
	float TerrainSizeZ;
	float TerrainHeightRange;
	float Pad;
}

cbuffer cbFoliage : register(b2)
{
	float FoliageDensity;
	int FoliageCountX;
	int FoliageCountZ;
	float Reserved;
}

SamplerState Sampler : register(s0);
Texture2D<float> HeightMapTexture : register(t0);
Texture2D<float> PerlinNoiseTexture : register(t1);

float GetHeightFromHeightMap(float2 XZ)
{
	float2 uv = CalculateHeightMapUV(float2(TerrainSizeX, TerrainSizeZ), XZ);
	float y_norm = HeightMapTexture.SampleLevel(Sampler, uv, 0);
	return (y_norm * TerrainHeightRange - TerrainHeightRange / 2.0f);
}

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	const float4x4 KInstanceWorld = float4x4(Input.InstanceWorld0, Input.InstanceWorld1, Input.InstanceWorld2, Input.InstanceWorld3);
	const float KInstanceX = KInstanceWorld._m30;
	const float KInstanceZ = KInstanceWorld._m32;
	const float2 KInstanceXZ = float2(KInstanceX, KInstanceZ);
	const float KHeight = GetHeightFromHeightMap(KInstanceXZ);
	const float KHeightXMinus = GetHeightFromHeightMap(KInstanceXZ + float2(-1, 0));
	const float KHeightXPlus = GetHeightFromHeightMap(KInstanceXZ + float2(+1, 0));
	const float KHeightZMinus = GetHeightFromHeightMap(KInstanceXZ + float2(0, -1));
	const float KHeightZPlus = GetHeightFromHeightMap(KInstanceXZ + float2(0, +1));
	const float KHeightAroundAverage = (KHeightXMinus + KHeightXPlus + KHeightZMinus + KHeightZPlus) * 0.25f;
	const float KHeightDifference = KHeightAroundAverage - KHeight;

	float XDifference = KHeightXPlus - KHeightXMinus;
	float ZRotationAngle = atan(XDifference / 2.0f);
	float ZDifference = KHeightZPlus - KHeightZMinus;
	float XRotationAngle = atan(ZDifference / 2.0f);

	float4x4 InstanceWorldWithoutXZTranslation = KInstanceWorld;
	InstanceWorldWithoutXZTranslation._m30 = InstanceWorldWithoutXZTranslation._m32 = 0.0f;

	float4 InstancePosition = mul(Input.Position, InstanceWorldWithoutXZTranslation);
	InstancePosition = RotateAroundAxisZ(InstancePosition, ZRotationAngle);
	InstancePosition = RotateAroundAxisX(InstancePosition, -XRotationAngle);
	InstancePosition.x += KInstanceWorld._m30;
	InstancePosition.z += KInstanceWorld._m32;
	InstancePosition.y += KHeight;
	//InstancePosition.y += KHeight + KHeightDifference * 0.0625f;

	Output.WorldPosition = InstancePosition;
	Output.Position = mul(InstancePosition, ViewProjection);

	const float KInstancePerlinU = ((float)TerrainSizeX * 0.5f + KInstanceX) / TerrainSizeX;
	const float KInstancePerlinV = ((float)TerrainSizeZ * 0.5f + KInstanceZ) / TerrainSizeZ;
	const float2 KInstancePerlinUV = float2(KInstancePerlinU, KInstancePerlinV);
	const float KFoliageDensityThreshold = 0.2f;
	float PerlinNoise = PerlinNoiseTexture.SampleLevel(Sampler, KInstancePerlinUV, 0);
	if (PerlinNoise + FoliageDensity + KFoliageDensityThreshold < 1.0f)
	{
		Output.Position.w = 0.0f;
	}

	Output.Color = Input.Color;
	Output.UV = Input.UV;

	float4 InstanceNormal = CalculateHeightMapNormal(KHeightXMinus, KHeightXPlus, KHeightZMinus, KHeightZPlus);
	Output.WorldNormal = normalize(mul(InstanceNormal, World));
	float4 ResultBitangent = normalize(float4(cross(InstanceNormal.xyz, Input.Tangent.xyz), 0));
	float4 ResultTangent = normalize(float4(cross(ResultBitangent.xyz, InstanceNormal.xyz), 0));
	Output.WorldTangent = normalize(mul(ResultTangent, World));
	Output.WorldBitangent = normalize(mul(ResultBitangent, World));

	Output.bUseVertexColor = 0;

	return Output;
}