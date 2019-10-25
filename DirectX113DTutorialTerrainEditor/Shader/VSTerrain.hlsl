#include "Header.hlsli"

cbuffer cbSpace : register(b0)
{
	float4x4 WVP;
	float4x4 World;
}

cbuffer cbTerrain : register(b1)
{
	float TerrainSizeX;
	float TerrainSizeZ;
	float TerrainHeightRange;
	float Pad;
}

SamplerState CurrentSampler : register(s0);
Texture2D<float> HeightMapTexture : register(t0);

float GetHeightFromHeightMap(float2 XZ)
{
	float2 uv = float2((XZ.x + (TerrainSizeX / 2.0f)) / TerrainSizeX, (-XZ.y + (TerrainSizeZ / 2.0f)) / TerrainSizeZ);
	float y_norm = HeightMapTexture.SampleLevel(CurrentSampler, uv, 0);
	return (y_norm * TerrainHeightRange - TerrainHeightRange / 2.0f);
}

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	float4 ResultPosition = input.Position;
	ResultPosition.y = GetHeightFromHeightMap(ResultPosition.xz);

	output.Position = mul(ResultPosition, WVP);
	output.WorldPosition = mul(ResultPosition, World);
	output.Color = input.Color;
	output.UV = input.UV;

	float HeightXMinus = GetHeightFromHeightMap(ResultPosition.xz + float2(-1, 0));
	float HeightXPlus = GetHeightFromHeightMap(ResultPosition.xz + float2(+1, 0));
	float HeightZMinus = GetHeightFromHeightMap(ResultPosition.xz + float2(0, -1));
	float HeightZPlus = GetHeightFromHeightMap(ResultPosition.xz + float2(0, +1));
	float4 ResultNormal = normalize(float4(HeightXMinus - HeightXPlus, 2.0f, HeightZMinus - HeightZPlus, 0));
	output.WorldNormal = normalize(mul(ResultNormal, World));

	float4 ResultBitangent = normalize(float4(cross(ResultNormal.xyz, input.Tangent.xyz), 0));
	float4 ResultTangent = normalize(float4(cross(ResultBitangent.xyz, ResultNormal.xyz), 0));
	output.WorldTangent = normalize(mul(ResultTangent, World));
	output.WorldBitangent = normalize(mul(ResultBitangent, World));

	output.bUseVertexColor = 0;

	return output;
}