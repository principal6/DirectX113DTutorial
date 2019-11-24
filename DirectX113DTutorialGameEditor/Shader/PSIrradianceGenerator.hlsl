#include "Deferred.hlsli"

SamplerState CubemapSampler : register(s0);
TextureCube CubemapTexture : register(t0);

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	// @important: this function is based on DirectX axis.
	//
	// y (up)
	// |  / z (forward)
	// | /
	// |/_____ x (right)
	//

	// This Normal vector corresponds to CubeSpaceY axis
	float3 Normal = normalize(Input.TexCoord.xyz);

	// This way we don't need to know cube face's ID.
	float3 UpVector = abs(Normal.y) < 0.999 ? float3(0, 1, 0) : float3(0, 0, -1);

	float3 CubeSpaceX = normalize(cross(UpVector, Normal));
	float3 CubeSpaceZ = cross(CubeSpaceX, Normal);

	float4 Irradiance = float4(0, 0, 0, 1);
	uint SampleCount = 0;
	for (float Theta = 0.0; Theta < KPIDIV2; Theta += 0.0078125)
	{
		for (float Phi = 0.0; Phi < K2PI; Phi += 0.015625)
		{
			float3 TangentSpaceDirection = float3(sin(Theta) * cos(Phi), cos(Theta), sin(Theta) * sin(Phi));
			float3 CubeSpaceDirection = TangentSpaceDirection.x * CubeSpaceX + TangentSpaceDirection.y * Normal + TangentSpaceDirection.z * CubeSpaceZ;

			Irradiance += CubemapTexture.SampleLevel(CubemapSampler, CubeSpaceDirection, 3) * cos(Theta) * sin(Theta);
			++SampleCount;
		}
	}

	Irradiance = Irradiance * KPI / (float)SampleCount;

	// Exposure tone mapping for raw albedo
	Irradiance.rgb = float3(1.0, 1.0, 1.0) - exp(-Irradiance.rgb * 0.6);

	return Irradiance;
}