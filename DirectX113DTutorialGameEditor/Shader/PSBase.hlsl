#include "HBase.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D DiffuseTexture : register(t0);
//Texture2D NormalTexture : register(t5);
Texture2D OpacityTexture : register(t10);

cbuffer cbFlags : register(b0)
{
	bool UseTexture;
	bool UseLighting;
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
	bool2	Pad;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 AmbientColor = float4(MaterialAmbient, 1);
	float4 DiffuseColor = float4(MaterialDiffuse, 1);
	float4 SpecularColor = float4(MaterialSpecular, 1);
	float Opacity = 1.0f;
	
	if (UseTexture == true)
	{
		if (bHasDiffuseTexture == true)
		{
			AmbientColor = DiffuseColor = SpecularColor = DiffuseTexture.Sample(CurrentSampler, input.UV.xy);
		}
		
		if (bHasOpacityTexture == true)
		{
			Opacity = OpacityTexture.Sample(CurrentSampler, input.UV.xy).r;
		}
	}
	DiffuseColor.xyz *= DiffuseColor.xyz;

	float4 Result = DiffuseColor;
	if (UseLighting == true)
	{
		Result = CalculateAmbient(AmbientColor, AmbientLightColor, AmbientLightIntensity);

		float4 Directional = CalculateDirectional(DiffuseColor, SpecularColor, SpecularExponent, SpecularIntensity,
			DirectionalLightColor, DirectionalLightDirection, normalize(EyePosition - input.WorldPosition), normalize(input.WorldNormal));

		// Directional Light의 위치가 지평선에 가까워질수록 빛의 세기를 약하게 한다.
		float Dot = dot(DirectionalLightDirection, KUpDirection);
		Directional.xyz *= pow(Dot, 0.6f);

		Result += Directional;
	}

	if (input.bUseVertexColor != 0)
	{
		return input.Color;
	}

	if (bHasOpacityTexture == true)
	{
		Result.a *= Opacity;
	}

	return Result;
}