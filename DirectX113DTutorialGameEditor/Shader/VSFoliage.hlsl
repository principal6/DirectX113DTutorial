#include "Foliage.hlsli"
#include "iVSCBs.hlsli"

cbuffer cbTerrain : register(b1)
{
	float TerrainSizeX;
	float TerrainSizeZ;
	float TerrainHeightRange;
	float Time;
}

cbuffer cbWind : register(b2)
{
	float4 WindVelocity;
	float3 WindPosition;
	float WindRadius;
}

SamplerState Sampler : register(s0);
Texture2D<float> HeightMapTexture : register(t0);

float GetHeightFromHeightMap(float2 XZ)
{
	float2 uv = GetHeightMapUVFromPosition(float2(TerrainSizeX, TerrainSizeZ), XZ);
	float y_norm = HeightMapTexture.SampleLevel(Sampler, uv, 0);
	return (y_norm * TerrainHeightRange - TerrainHeightRange / 2.0f);
}

VS_FOLIAGE_OUTPUT main(VS_FOLIAGE_INPUT Input)
{
	VS_FOLIAGE_OUTPUT Output;

	const float4x4 KInstanceWorld = float4x4(Input.InstanceWorld0, Input.InstanceWorld1, Input.InstanceWorld2, Input.InstanceWorld3);
	const float2 KInstanceXZ = float2(KInstanceWorld._m30, KInstanceWorld._m32);
	const float KHeight = GetHeightFromHeightMap(KInstanceXZ);
	const float KHeightXMinus = GetHeightFromHeightMap(KInstanceXZ + float2(-1, 0));
	const float KHeightXPlus = GetHeightFromHeightMap(KInstanceXZ + float2(+1, 0));
	const float KHeightZMinus = GetHeightFromHeightMap(KInstanceXZ + float2(0, -1));
	const float KHeightZPlus = GetHeightFromHeightMap(KInstanceXZ + float2(0, +1));
	const float KZRotationAngle = atan((KHeightXPlus - KHeightXMinus) / 2.0f);
	const float KXRotationAngle = atan((KHeightZPlus - KHeightZMinus) / 2.0f);

	float4 InstancePosition = Input.Position;
	InstancePosition.xyz = mul(Input.Position.xyz, (float3x3)KInstanceWorld);
	InstancePosition = RotateAroundAxisZ(InstancePosition, KZRotationAngle);
	InstancePosition = RotateAroundAxisX(InstancePosition, -KXRotationAngle);
	InstancePosition.x += KInstanceWorld._m30;
	InstancePosition.z += KInstanceWorld._m32;

	Output.WorldNormal = normalize(mul(Input.Normal, KInstanceWorld));

	if (Input.TexCoord.y < 0.2f) // Upper vertices
	{
		float Distance = distance(InstancePosition.xz, WindPosition.xz);
		if (Distance <= WindRadius)
		{
			const float KWindDisplacementFactor = 0.0625f;
			float3 WindDirection = normalize(WindVelocity.xyz);
			float NormalizedDistance = Distance / WindRadius;
			InstancePosition.x += WindVelocity.x * (1.0f - NormalizedDistance) * (sin(Time * K2PI) * 0.125f + 0.875f) * KWindDisplacementFactor;
			InstancePosition.z += WindVelocity.z * (1.0f - NormalizedDistance) * (sin(Time * K2PI) * 0.125f + 0.875f) * KWindDisplacementFactor;
		}
	}

	InstancePosition.y += KHeight;

	Output.WorldPosition = InstancePosition;
	Output.Position = mul(InstancePosition, ViewProjection);
	Output.TexCoord = Input.TexCoord;
	//Output.WorldNormal = normalize(mul(Input.Normal, KInstanceWorld));

	return Output;
}