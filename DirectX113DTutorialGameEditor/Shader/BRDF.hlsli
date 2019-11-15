#include "Shared.hlsli"

static float4 CalculateClassicalAmbient(float4 AmbientColor, float3 AmbientLightColor, float AmbientLightIntensity)
{
	return float4(AmbientColor.xyz * AmbientLightColor * AmbientLightIntensity, AmbientColor.a);
}

static float4 CalculateClassicalDirectional(float4 DiffuseColor, float4 SpecularColor, float SpecularExponent, float SpecularIntensity,
	float4 LightColor, float4 LightDirection, float4 ToEye, float4 Normal)
{
	float NDotL = saturate(dot(LightDirection, Normal));
	float4 PhongDiffuse = DiffuseColor * LightColor * NDotL;

	float4 H = normalize(ToEye + LightDirection);
	float NDotH = saturate(dot(H, Normal));
	float SpecularPower = 0;
	if (NDotH != 0) SpecularPower = pow(NDotH, SpecularExponent);
	float4 BlinnSpecular = SpecularColor * LightColor * SpecularPower * SpecularIntensity;

	float4 Result = PhongDiffuse + BlinnSpecular;
	return Result;
}