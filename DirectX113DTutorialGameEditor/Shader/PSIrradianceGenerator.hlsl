#include "Deferred.hlsli"

SamplerState CubemapSampler : register(s0);
TextureCube CubemapTexture : register(t0);

cbuffer cbCubemap : register(b0)
{
	uint FaceID;
	float3 Pads;
};

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float3 Normal = normalize(Input.TexCoord.xyz);

	float3 FaceUp = float3(+1, 0, 0); // X+
	if (FaceID == 1) FaceUp = float3(-1, 0, 0); // X-
	if (FaceID == 2) FaceUp = float3(0, +1, 0); // Y+
	if (FaceID == 3) FaceUp = float3(0, -1, 0); // Y-
	if (FaceID == 4) FaceUp = float3(0, 0, +1); // Z+
	if (FaceID == 5) FaceUp = float3(0, 0, -1); // Z-
	
	float3 Right = normalize(cross(Normal, FaceUp));
	float3 Up = normalize(cross(Normal, Right));

	float4 Irradiance = float4(0, 0, 0, 0);
	uint SampleCount = 0;
	for (float Theta = 0.0; Theta < KPIDIV2; Theta += 0.1)
	{
		for (float Phi = 0.0; Phi < K2PI; Phi += 0.025)
		{
			float3 SphericalDirection = float3(sin(Theta) * cos(Phi), cos(Theta), sin(Theta) * sin(Phi));
			float3 CubeSpaceDirection = SphericalDirection.x * Right + SphericalDirection.y * Normal + SphericalDirection.z * Up;

			Irradiance += CubemapTexture.SampleLevel(CubemapSampler, CubeSpaceDirection, 2) * cos(Theta) * sin(Theta);
			++SampleCount;
		}
	}

	return Irradiance * KPI / (float)SampleCount;
}