#include "Base.hlsli"
#include "BRDF.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D DiffuseTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D OpacityTexture : register(t2);
Texture2D SpecularIntensityTexture : register(t3);
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
	float4	DirectionalLightColor;
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
	bool	bHasDiffuseTexture;

	bool	bHasNormalTexture;
	bool	bHasOpacityTexture;
	bool	bHasSpecularIntensityTexture;
	bool	Reserved;
}

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float4 AmbientColor = float4(MaterialAmbientColor, 1);
	float4 DiffuseColor = float4(MaterialDiffuseColor, 1);
	float4 SpecularColor = float4(MaterialSpecularColor, 1);
	float4 WorldNormal = normalize(Input.WorldNormal);
	float SpecularIntensity = MaterialSpecularIntensity;
	float Opacity = 1.0f;
	
	if (bUseTexture == true)
	{
		if (bHasDiffuseTexture == true)
		{
			AmbientColor = DiffuseColor = SpecularColor = DiffuseTexture.Sample(CurrentSampler, Input.UV.xy);
		}

		if (bHasNormalTexture == true)
		{
			WorldNormal = normalize((NormalTexture.Sample(CurrentSampler, Input.UV.xy) * 2.0f) - 1.0f);

			float3x3 TextureSpace = float3x3(Input.WorldTangent.xyz, Input.WorldBitangent.xyz, Input.WorldNormal.xyz);
			WorldNormal = normalize(float4(mul(WorldNormal.xyz, TextureSpace), 0.0f));
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

		if (bHasSpecularIntensityTexture == true)
		{
			SpecularIntensity = SpecularIntensityTexture.Sample(CurrentSampler, Input.UV.xy).r;
		}
	}

	// # Here we make sure that input RGB values are in linear-space!
	// (In this project, all textures are loaded with their values in gamma-space)
	if (bHasDiffuseTexture == true)
	{
		// # Convert gamma-space RGB to linear-space RGB (sRGB)
		DiffuseColor.xyz = pow(DiffuseColor.xyz, 2.2);
		AmbientColor.xyz = pow(AmbientColor.xyz, 2.2);
		SpecularColor.xyz = pow(SpecularColor.xyz, 2.2);
	}

	float4 OutputColor = DiffuseColor;
	if (bUseLighting == true)
	{
		float4 Ambient = CalculateClassicalAmbient(AmbientColor, AmbientLightColor, AmbientLightIntensity);
		float4 Directional = CalculateClassicalDirectional(DiffuseColor, SpecularColor, MaterialSpecularExponent, SpecularIntensity,
			DirectionalLightColor, DirectionalLightDirection, normalize(EyePosition - Input.WorldPosition), WorldNormal);

		// Directional Light의 위치가 지평선에 가까워질수록 빛의 세기를 약하게 한다.
		float Dot = dot(DirectionalLightDirection, KUpDirection);
		Directional.xyz *= pow(Dot, 0.6f);

		OutputColor = Ambient + Directional;
	}

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB (sRGB) to gamma-space RGB
	OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	if (Input.bUseVertexColor != 0) OutputColor = Input.Color;
	if (bHasOpacityTexture == true) OutputColor.a *= Opacity;
	
	return OutputColor;
}