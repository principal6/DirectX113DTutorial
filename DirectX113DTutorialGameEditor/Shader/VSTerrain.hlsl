#include "Terrain.hlsli"
#include "iVSCBs.hlsli"

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
	float2 uv = GetHeightMapUVFromPosition(float2(TerrainSizeX, TerrainSizeZ), XZ);
	float y_norm = HeightMapTexture.SampleLevel(HeightMapSampler, uv, 0);
	return (y_norm * TerrainHeightRange - TerrainHeightRange / 2.0f);
}

VS_OUTPUT main(VS_INPUT Input)
{
	VS_OUTPUT Output;

	float2 HeightMapUV = GetHeightMapUVFromPosition(float2(TerrainSizeX, TerrainSizeZ), Input.Position.xz);

	float Height = GetHeightFromHeightMap(Input.Position.xz);
	float HeightXMinus = GetHeightFromHeightMap(Input.Position.xz + float2(-1, 0));
	float HeightXPlus = GetHeightFromHeightMap(Input.Position.xz + float2(+1, 0));
	float HeightZMinus = GetHeightFromHeightMap(Input.Position.xz + float2(0, -1));
	float HeightZPlus = GetHeightFromHeightMap(Input.Position.xz + float2(0, +1));
	
	Output.WorldPosition = mul(float4(Input.Position.x, Height, Input.Position.z, 1), World);
	Output.Position = mul(Output.WorldPosition, ViewProjection);

	Output.Color = Input.Color;
	Output.TexCoord = Input.TexCoord;

	float4 ResultNormal = CalculateHeightMapNormal(HeightXMinus, HeightXPlus, HeightZMinus, HeightZPlus);
	Output.WorldNormal = normalize(mul(ResultNormal, World));

	float4 ResultBitangent = float4(normalize(cross(ResultNormal.xyz, Input.Tangent.xyz)), 0);
	float4 ResultTangent = float4(cross(ResultBitangent.xyz, ResultNormal.xyz), 0);
	Output.WorldTangent = normalize(mul(ResultTangent, World));
	Output.WorldBitangent = normalize(mul(ResultBitangent, World));

	Output.bUseVertexColor = 0;
	Output.IsHighlighted = Input.IsHighlighted;

#ifndef DEBUG_SHADER
	Output.InstanceID = Input.InstanceID;
#endif

	return Output;
}