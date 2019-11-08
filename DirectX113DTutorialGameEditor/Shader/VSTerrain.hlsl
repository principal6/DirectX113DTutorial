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

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	float4 ResultPosition = input.Position;
	ResultPosition.y = GetHeightFromHeightMap(ResultPosition.xz);
	output.WorldPosition = mul(ResultPosition, World);
	output.Position = mul(output.WorldPosition, ViewProjection);

	output.Color = input.Color;
	output.UV = input.UV;

	float HeightXMinus = GetHeightFromHeightMap(ResultPosition.xz + float2(-1, 0));
	float HeightXPlus = GetHeightFromHeightMap(ResultPosition.xz + float2(+1, 0));
	float HeightZMinus = GetHeightFromHeightMap(ResultPosition.xz + float2(0, -1));
	float HeightZPlus = GetHeightFromHeightMap(ResultPosition.xz + float2(0, +1));
	float4 ResultNormal = CalculateHeightMapNormal(HeightXMinus, HeightXPlus, HeightZMinus, HeightZPlus);
	output.WorldNormal = normalize(mul(ResultNormal, World));

	float4 ResultBitangent = normalize(float4(cross(ResultNormal.xyz, input.Tangent.xyz), 0));
	float4 ResultTangent = normalize(float4(cross(ResultBitangent.xyz, ResultNormal.xyz), 0));
	output.WorldTangent = normalize(mul(ResultTangent, World));
	output.WorldBitangent = normalize(mul(ResultBitangent, World));

	output.bUseVertexColor = 0;

	return output;
}