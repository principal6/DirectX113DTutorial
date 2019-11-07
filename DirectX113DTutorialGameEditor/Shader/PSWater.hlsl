#include "Terrain.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D WaterNormalTexture : register(t0);

cbuffer cbTime : register(b0)
{
	float Time;
	float3 Pads;
}

cbuffer cbLights : register(b1)
{
	float4	DirectionalLightDirection;
	float4	DirectionalLightColor;
	float3	AmbientLightColor;
	float	AmbientLightIntensity;
	float4	EyePosition;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 Albedo = input.Color;

	float3x3 TextureSpace = float3x3(input.WorldTangent.xyz, input.WorldBitangent.xyz, input.WorldNormal.xyz);
	float2 ResultUV = input.UV.xy - float2(0, Time);
	float4 ResultNormal = normalize((WaterNormalTexture.Sample(CurrentSampler, ResultUV) * 2.0f) - 1.0f);
	ResultNormal = normalize(float4(mul(ResultNormal.xyz, TextureSpace), 0.0f));

	// # Gamma correction
	Albedo.xyz = pow(Albedo.xyz, 2.0f);

	float4 ResultColor = Albedo;
	{
		float4 Ambient = CalculateAmbient(Albedo, AmbientLightColor, AmbientLightIntensity);
		float4 Directional = CalculateDirectional(Albedo, float4(1, 1, 1, 1), 64.0f, 1.0f,
			DirectionalLightColor, DirectionalLightDirection, normalize(EyePosition - input.WorldPosition), normalize(ResultNormal));

		ResultColor.xyz = Ambient.xyz + Directional.xyz;
	}

	// # Gamma correction
	ResultColor.xyz = pow(ResultColor.xyz, 0.5f);

	return ResultColor;
}