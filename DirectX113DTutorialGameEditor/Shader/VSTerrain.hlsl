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
	float Time;
}

SamplerState HeightMapSampler : register(s0);
Texture2D<float> HeightMapTexture : register(t0);

float GetHeightFromHeightMap(float2 XZ)
{
	float2 uv = CalculateHeightMapUV(float2(TerrainSizeX, TerrainSizeZ), XZ);
	float y_norm = HeightMapTexture.SampleLevel(HeightMapSampler, uv, 0);
	return (y_norm * TerrainHeightRange - TerrainHeightRange / 2.0f);
}

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	float4 ResultPosition = Input.Position;
	ResultPosition.y = GetHeightFromHeightMap(ResultPosition.xz);
	Output.WorldPosition = mul(ResultPosition, World);
	Output.Position = mul(Output.WorldPosition, ViewProjection);

	Output.Color = Input.Color;
	Output.UV = Input.UV;

	float HeightXMinus = GetHeightFromHeightMap(ResultPosition.xz + float2(-1, 0));
	float HeightXPlus = GetHeightFromHeightMap(ResultPosition.xz + float2(+1, 0));
	float HeightZMinus = GetHeightFromHeightMap(ResultPosition.xz + float2(0, -1));
	float HeightZPlus = GetHeightFromHeightMap(ResultPosition.xz + float2(0, +1));
	float4 ResultNormal = CalculateHeightMapNormal(HeightXMinus, HeightXPlus, HeightZMinus, HeightZPlus);
	Output.WorldNormal = normalize(mul(ResultNormal, World));

	float4 ResultBitangent = normalize(float4(cross(ResultNormal.xyz, Input.Tangent.xyz), 0));
	float4 ResultTangent = normalize(float4(cross(ResultBitangent.xyz, ResultNormal.xyz), 0));
	Output.WorldTangent = normalize(mul(ResultTangent, World));
	Output.WorldBitangent = normalize(mul(ResultBitangent, World));

	Output.bUseVertexColor = 0;

	return Output;
}