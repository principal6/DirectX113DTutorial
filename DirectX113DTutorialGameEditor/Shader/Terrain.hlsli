#include "Base.hlsli"

static const float KDistanceThreshold = 12.0;
static float ThresholdDistance(float Distance)
{
	if (Distance < KDistanceThreshold) Distance = 1.0;
	if (Distance >= KDistanceThreshold) Distance -= (KDistanceThreshold - 1.0);
	return Distance;
}

static float2 GetHeightMapUVFromPosition(float2 TerrainSize, float2 XZ)
{
	return float2((XZ.x + (TerrainSize.x / 2.0f)) / TerrainSize.x, (-XZ.y + (TerrainSize.y / 2.0f)) / TerrainSize.y);
}

static float4 CalculateHeightMapNormal(float HeightXMinus, float HeightXPlus, float HeightZMinus, float HeightZPlus)
{
	return normalize(float4(HeightXMinus - HeightXPlus, 2.0f, HeightZMinus - HeightZPlus, 0));
}