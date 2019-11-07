#include "Foliage.hlsli"

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

SamplerState Sampler : register(s0);
Texture2D<float> HeightMapTexture : register(t0);

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

	const float KXDifference = KHeightXPlus - KHeightXMinus;
	const float KZRotationAngle = atan(KXDifference / 2.0f);
	const float KZDifference = KHeightZPlus - KHeightZMinus;
	const float KXRotationAngle = atan(KZDifference / 2.0f);

	float4 InstancePosition = Input.Position;
	InstancePosition.xyz = mul(Input.Position.xyz, (float3x3)KInstanceWorld);
	InstancePosition = RotateAroundAxisZ(InstancePosition, KZRotationAngle);
	InstancePosition = RotateAroundAxisX(InstancePosition, -KXRotationAngle);
	InstancePosition.x += KInstanceWorld._m30;
	InstancePosition.z += KInstanceWorld._m32;
	InstancePosition.y += KHeight;

	Output.WorldPosition = InstancePosition;
	Output.Position = mul(InstancePosition, ViewProjection);
	Output.UV = Input.UV;
	Output.WorldNormal = normalize(mul(Input.Normal, KInstanceWorld));

	return Output;
}