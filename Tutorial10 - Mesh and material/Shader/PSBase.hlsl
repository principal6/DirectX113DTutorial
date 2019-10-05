#include "Header.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D CurrentTexture2D : register(t0);

cbuffer cbFlags : register(b0)
{
	bool UseTexture;
	bool UseLighting;
	bool2 Pads;
}

cbuffer cbLights : register(b1)
{
	float4	DirectionalDirection;
	float4	DirectionalColor;
	float3	AmbientColor;
	float	AmbientIntensity;
}

cbuffer cbMaterial : register(b2)
{
	float3	MaterialAmbient;
	float	SpecularExponent;
	float3	MaterialDiffuse;
	float	SpecularIntensity;
	float3	MaterialSpecular;
	float	Pad;
}

cbuffer cbEye : register(b3)
{
	float4	EyePosition;
}

float4 CalculateAmbient()
{
	return float4(MaterialAmbient * AmbientColor * AmbientIntensity, 1);
}

float4 CalculateDirectional(float4 DiffuseColor, float4 ToEye, float4 Normal)
{
	float NDotL = saturate(dot(DirectionalDirection, Normal));
	float4 PhongDiffuse = DiffuseColor * DirectionalColor * NDotL;

	float4 H = normalize(ToEye + DirectionalDirection);
	float NDotH = saturate(dot(H, Normal));
	float SpecularPower = pow(NDotH, SpecularExponent);
	float4 BlinnSpecular = float4(MaterialSpecular * DirectionalColor.xyz * SpecularPower * SpecularIntensity, 1);

	return PhongDiffuse + BlinnSpecular;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 DiffuseColor = float4(MaterialDiffuse, 1);
	if (UseTexture == true)
	{
		DiffuseColor = CurrentTexture2D.Sample(CurrentSampler, input.UV);
	}
	DiffuseColor.xyz *= DiffuseColor.xyz;

	float4 Result = DiffuseColor;
	if (UseLighting == true)
	{
		Result = CalculateAmbient();
		Result += CalculateDirectional(DiffuseColor, normalize(EyePosition - input.WorldPosition), normalize(input.WorldNormal));
	}

	return Result;
}