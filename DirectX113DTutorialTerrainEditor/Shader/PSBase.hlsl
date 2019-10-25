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
	float4	EyePosition;
}

cbuffer cbMaterial : register(b2)
{
	float3	MaterialAmbient;
	float	SpecularExponent;
	float3	MaterialDiffuse;
	float	SpecularIntensity;
	float3	MaterialSpecular;
	bool	bHasTexture;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 AmbientColor = float4(MaterialAmbient, 1);
	float4 DiffuseColor = float4(MaterialDiffuse, 1);
	float4 SpecularColor = float4(MaterialSpecular, 1);
	
	if (UseTexture == true && bHasTexture == true)
	{
		AmbientColor = DiffuseColor = SpecularColor = CurrentTexture2D.Sample(CurrentSampler, input.UV.xy);
	}
	DiffuseColor.xyz *= DiffuseColor.xyz;

	float4 Result = DiffuseColor;
	if (UseLighting == true)
	{
		Result = CalculateAmbient(AmbientColor, AmbientLightColor, AmbientLightIntensity);

		float4 Directional = CalculateDirectional(DiffuseColor, SpecularColor, SpecularExponent, SpecularIntensity,
			DirectionalLightColor, DirectionalLightDirection, normalize(EyePosition - input.WorldPosition), normalize(input.WorldNormal));

		// 해나 달의 위치가 지평선에 가까워질수록 빛의 세기를 약하게 한다.
		float Dot = dot(DirectionalLightDirection, float4(0, 1, 0, 0));
		Directional.xyz *= pow(Dot, 0.6f);

		Result += Directional;
	}

	if (input.bUseVertexColor != 0)
	{
		return input.Color;
	}

	return Result;
}