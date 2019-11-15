#include "Foliage.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D DiffuseTexture : register(t0);
// Normal texture slot
Texture2D OpacityTexture : register(t2);

cbuffer cbFlags : register(b0)
{
	bool bUseTexture;
	bool bUseLighting;
	bool2 Pads;
}

cbuffer cbLights : register(b1)
{
	float4	DirectionalLightDirection;
	float4	DirectionalLightColor;
	float3	AmbientLightColor;
	float	AmbientLightIntensity;
	float4	EyePosition;
}

cbuffer cbMaterial : register(b2)
{
	float3	MaterialAmbient;
	float	SpecularExponent;
	float3	MaterialDiffuse;
	float	SpecularIntensity;
	float3	MaterialSpecular;
	bool	bHasDiffuseTexture;

	bool	bHasNormalTexture;
	bool	bHasOpacityTexture;
	bool2	Pads2;
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
		float4 Ambient = CalculateAmbient(Albedo, AmbientLightColor, AmbientLightIntensity);
		float4 Directional = CalculateDirectional(Albedo, Albedo, 1.0f, 0.0f,
			DirectionalLightColor, DirectionalLightDirection, normalize(EyePosition - Input.WorldPosition), normalize(Input.WorldNormal));

		// Directional Light의 위치가 지평선에 가까워질수록 빛의 세기를 약하게 한다.
		float Dot = dot(DirectionalLightDirection, KUpDirection);
		Directional.xyz *= pow(Dot, 0.6f);

		OutputColor = Ambient + Directional;
	}

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB (sRGB) to gamma-space RGB
	OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	if (bHasOpacityTexture == true) OutputColor.a *= Opacity;

	return OutputColor;
}