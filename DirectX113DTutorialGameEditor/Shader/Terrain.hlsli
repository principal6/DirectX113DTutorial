#include "Base.hlsli"

static float2 CalculateHeightMapUV(float2 TerrainSize, float2 XZ)
{
	return float2((XZ.x + (TerrainSize.x / 2.0f)) / TerrainSize.x, (-XZ.y + (TerrainSize.y / 2.0f)) / TerrainSize.y);
}

static float4 CalculateHeightMapNormal(float HeightXMinus, float HeightXPlus, float HeightZMinus, float HeightZPlus)
{
	return normalize(float4(HeightXMinus - HeightXPlus, 2.0f, HeightZMinus - HeightZPlus, 0));
}