#include "Header.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D WaterNormalTexture : register(t0);

cbuffer cbCamera : register(b0)
{
	float4	EyePosition;
}

float4 CalculateDirectional(float4 DiffuseColor, float4 SpecularColor, float4 ToEye, float4 Normal)
{
	const float4 KLightDirection = normalize(float4(1, 1, 0, 1));
	const float4 KLightColor = float4(1, 1, 1, 1);
	const float KSpecularExponent = 20.0f;
	const float KSpecularIntensity = 4.0f;

	float NDotL = saturate(dot(KLightDirection, Normal));
	float4 PhongDiffuse = DiffuseColor * KLightColor * NDotL;

	float4 H = normalize(ToEye + KLightDirection);
	float NDotH = saturate(dot(H, Normal));
	float SpecularPower = pow(NDotH, KSpecularExponent);
	float4 BlinnSpecular = SpecularColor * KLightColor * SpecularPower * KSpecularIntensity;

	float4 Result = PhongDiffuse + BlinnSpecular;
	return Result;
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 ResultAlbedo = input.Color;

	float3x3 TextureSpace = float3x3(input.WorldTangent.xyz, input.WorldBitangent.xyz, input.WorldNormal.xyz);
	float4 ResultNormal = normalize((WaterNormalTexture.Sample(CurrentSampler, input.UV.xy) * 2.0f) - 1.0f);
	ResultNormal = normalize(float4(mul(ResultNormal.xyz, TextureSpace), 0.0f));

	// Fixed directional light (for edit mode)
	float4 Directional = CalculateDirectional(ResultAlbedo, float4(1, 1, 1, 1), normalize(EyePosition - input.WorldPosition), ResultNormal);
	ResultAlbedo = Directional;

	return ResultAlbedo;
}