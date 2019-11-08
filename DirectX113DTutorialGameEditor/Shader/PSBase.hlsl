#include "Base.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D DiffuseTexture : register(t0);
//Texture2D NormalTexture : register(t5);
Texture2D OpacityTexture : register(t10);

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

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 AmbientColor = float4(MaterialAmbient, 1);
	float4 DiffuseColor = float4(MaterialDiffuse, 1);
	float4 SpecularColor = float4(MaterialSpecular, 1);
	float Opacity = 1.0f;
	
	if (bUseTexture == true)
	{
		if (bHasDiffuseTexture == true)
		{
			AmbientColor = DiffuseColor = SpecularColor = DiffuseTexture.Sample(CurrentSampler, input.UV.xy);
		}
		
		if (bHasOpacityTexture == true)
		{
			float4 Sampled = OpacityTexture.Sample(CurrentSampler, input.UV.xy);
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

	// # Gamma correction
	DiffuseColor.xyz = pow(DiffuseColor.xyz, 2.0f);
	AmbientColor.xyz = pow(AmbientColor.xyz, 2.0f);
	SpecularColor.xyz = pow(SpecularColor.xyz, 2.0f);

	float4 ResultColor = DiffuseColor;
	if (bUseLighting == true)
	{
		float4 Ambient = CalculateAmbient(AmbientColor, AmbientLightColor, AmbientLightIntensity);
		float4 Directional = CalculateDirectional(DiffuseColor, SpecularColor, SpecularExponent, SpecularIntensity,
			DirectionalLightColor, DirectionalLightDirection, normalize(EyePosition - input.WorldPosition), normalize(input.WorldNormal));

		// Directional Light�� ��ġ�� ���򼱿� ����������� ���� ���⸦ ���ϰ� �Ѵ�.
		float Dot = dot(DirectionalLightDirection, KUpDirection);
		Directional.xyz *= pow(Dot, 0.6f);

		ResultColor = Ambient + Directional;
	}

	// # Gamma correction
	ResultColor.xyz = pow(ResultColor.xyz, 0.5f);

	if (input.bUseVertexColor != 0) ResultColor = input.Color;
	if (bHasOpacityTexture == true) ResultColor.a *= Opacity;
	
	return ResultColor;
}