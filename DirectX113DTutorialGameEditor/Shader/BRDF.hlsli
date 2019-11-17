#include "Shared.hlsli"

static const float3 KF_Dielectric = float3(0.04, 0.04, 0.04); // 0.04 is simplified dielectric F0 value
static const float3 KMonochromatic = float3(0.2126, 0.7152, 0.0722);

static float3 DiffuseBRDF_Lambertian(float3 DiffuseColor)
{
	return DiffuseColor / KPI;
}

static float3 Fresnel_Schlick(float3 F0, float HdotL)
{
	return F0 + (float3(1.0, 1.0, 1.0) - F0) * pow(1.0 - HdotL, 5.0);
}

// Monodirectional shadowing-masking function (S is either V or L)
static float G1_Smith(float NdotS, float Alpha)
{
	float Numerator = NdotS;
	float Denominator = NdotS * (1 - Alpha) + Alpha;
	return Numerator / Denominator;
}

// Bidirectional shadowing-masking function
static float G2_Smith(float NdotL, float NdotV, float Alpha)
{
	return G1_Smith(NdotL, Alpha) * G1_Smith(NdotV, Alpha);
}

// (S is either V or L)
static float G1_Smith_GGX(float NdotS, float Alpha)
{
	float AlphaSquare = Alpha * Alpha;

	float Numerator = 2 * NdotS;
	float Denominator = NdotS + sqrt(AlphaSquare + (1.0 - AlphaSquare) * NdotS * NdotS);
	return Numerator / Denominator;
}

static float G2_Smith_GGX(float NdotL, float NdotV, float Alpha)
{
	return G1_Smith_GGX(NdotL, Alpha) * G1_Smith_GGX(NdotV, Alpha);
}

// Isotropic
static float NormalDistribution_GGX(float Alpha, float NdotH)
{
	float NdotHSquare = NdotH * NdotH;
	float AlphaSquare = Alpha * Alpha;

	float Numerator = AlphaSquare;
	float Denominator = NdotHSquare * (AlphaSquare - 1.0) + 1.0;
	Denominator = KPI * Denominator * Denominator;
	return Numerator / Denominator;
}

static float3 SpecularBRDF_GGX(float3 F0, float NdotL, float NdotV, float NdotH, float HdotL, float Roughness)
{
	float Alpha = Roughness * Roughness;

	float3 F = Fresnel_Schlick(F0, HdotL);
	float G = G2_Smith(NdotL, NdotV, Alpha);
	float D = NormalDistribution_GGX(Alpha, NdotH);

	float3 Numerator = F * G * D;
	float Denominator = 4 * NdotL * NdotV; // Mathematically this number '4' should be 'PI', but practically '4' is not bad!
	return (Numerator / Denominator);
}

static float3 CalculateClassicalAmbient(float3 AmbientColor, float3 AmbientLightColor, float AmbientLightIntensity)
{
	return AmbientColor * AmbientLightColor * AmbientLightIntensity;
}

static float3 CalculateClassicalDirectional(float3 DiffuseColor, float3 SpecularColor, float SpecularExponent, float SpecularIntensity,
	float3 LightColor, float3 L, float3 V, float3 Normal)
{
	float NdotL = saturate(dot(L, Normal));
	float3 PhongDiffuse = DiffuseColor * LightColor * NdotL;

	float3 H = normalize(V + L);
	float NdotH = saturate(dot(H, Normal));
	float SpecularPower = 0;
	if (NdotH != 0) SpecularPower = pow(NdotH, SpecularExponent);
	float3 BlinnSpecular = SpecularColor * LightColor * SpecularPower * SpecularIntensity;

	float3 Result = PhongDiffuse + BlinnSpecular;
	return Result;
}