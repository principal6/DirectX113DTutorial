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
	float4	DirectionalLightDirection;
	float4	DirectionalLightColor;
	float3	AmbientLightColor;
	float	AmbientLightIntensity;
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

float4 CalculateAmbient(float4 AmbientColor)
{
	return float4(AmbientColor.xyz * AmbientLightColor * AmbientLightIntensity, 1);
}

float4 CalculateDirectional(float4 DiffuseColor, float4 SpecularColor, float4 ToEye, float4 Normal)
{
	float NDotL = saturate(dot(DirectionalLightDirection, Normal));
	float4 PhongDiffuse = DiffuseColor * DirectionalLightColor * NDotL;

	float4 H = normalize(ToEye + DirectionalLightDirection);
	float NDotH = saturate(dot(H, Normal));
	float SpecularPower = pow(NDotH, SpecularExponent);
	float4 BlinnSpecular = float4(SpecularColor.xyz * DirectionalLightColor.xyz * SpecularPower * SpecularIntensity, 1);

	float4 Result = PhongDiffuse + BlinnSpecular;

	// �س� ���� ��ġ�� ���򼱿� ����������� ���� ���⸦ ���ϰ� �Ѵ�.
	float Dot = dot(DirectionalLightDirection, float4(0, 1, 0, 0));
	Result.xyz *= pow(Dot, 0.6f);

	return Result;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 AmbientColor = float4(MaterialAmbient, 1);
	float4 DiffuseColor = float4(MaterialDiffuse, 1);
	float4 SpecularColor = float4(MaterialSpecular, 1);
	
	if (UseTexture == true)
	{
		AmbientColor = DiffuseColor = SpecularColor = CurrentTexture2D.Sample(CurrentSampler, input.UV);
	}
	DiffuseColor.xyz *= DiffuseColor.xyz;

	float4 Result = DiffuseColor;
	if (UseLighting == true)
	{
		Result = CalculateAmbient(AmbientColor);
		Result += CalculateDirectional(DiffuseColor, SpecularColor, normalize(EyePosition - input.WorldPosition), normalize(input.WorldNormal));
	}

	return Result;
}