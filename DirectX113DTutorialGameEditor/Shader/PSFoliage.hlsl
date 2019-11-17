#include "Foliage.hlsli"
#include "BRDF.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D DiffuseTexture : register(t0);
// Normal texture slot
Texture2D OpacityTexture : register(t2);
// Specular intensity texture slot
// Displacement texture slot

cbuffer cbFlags : register(b0)
{
	bool bUseTexture;
	bool bUseLighting;
	bool2 Pads;
}

cbuffer cbLight : register(b1)
{
	float4	DirectionalLightDirection;
	float3	DirectionalLightColor;
	float	Exposure;
	float3	AmbientLightColor;
	float	AmbientLightIntensity;
	float4	EyePosition;
}

cbuffer cbMaterial : register(b2)
{
	float3	MaterialAmbientColor;
	float	MaterialSpecularExponent;
	float3	MaterialDiffuseColor;
	float	MaterialSpecularIntensity;
	float3	MaterialSpecularColor;
	float	MaterialRoughness;

	float	MaterialMetalness;
	bool	bHasDiffuseTexture;
	bool	bHasNormalTexture;
	bool	bHasOpacityTexture;

	bool	bHasSpecularIntensityTexture;
	bool	bHasRoughnessTexture;
	bool	bHasMetalnessTexture;
	bool	Reserved;
}

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float4 Albedo = float4(1, 1, 1, 1);
	float Opacity = 1.0f;

	if (bUseTexture == true)
	{
		if (bHasDiffuseTexture == true)
		{
			Albedo = DiffuseTexture.Sample(CurrentSampler, Input.UV.xy);
		}

		if (bHasOpacityTexture == true)
		{
			float4 Sampled = OpacityTexture.Sample(CurrentSampler, Input.UV.xy);
			if (Sampled.r == Sampled.g && Sampled.g == Sampled.b)
			{
				Opacity = Sampled.r;
			}
			else
			{
				Opacity = Sampled.a;
			}
		}
	}

	// # Here we make sure that input RGB values are in linear-space!
	// (In this project, all textures are loaded with their values in gamma-space)
	if (bHasDiffuseTexture == true)
	{
		Albedo.xyz = pow(Albedo.xyz, 2.2);
	}

	float4 OutputColor = Albedo;
	if (bUseLighting == true)
	{
		float3 N = normalize(Input.WorldNormal).xyz;
		float3 L = DirectionalLightDirection.xyz;
		float3 V = normalize(EyePosition.xyz - Input.WorldPosition.xyz);

		float3 Ambient = CalculateClassicalAmbient(Albedo.xyz, AmbientLightColor, AmbientLightIntensity);
		float3 Directional = CalculateClassicalDirectional(Albedo.xyz, Albedo.xyz, 1.0f, 0.0f,
			DirectionalLightColor, L, V, N);

		// Directional Light의 위치가 지평선에 가까워질수록 빛의 세기를 약하게 한다.
		float Dot = dot(DirectionalLightDirection, KUpDirection);
		Directional.xyz *= pow(Dot, 0.6f);

		OutputColor.xyz = Ambient + Directional;
	}

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB (sRGB) to gamma-space RGB
	OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	if (bHasOpacityTexture == true) OutputColor.a *= Opacity;

	return OutputColor;
}