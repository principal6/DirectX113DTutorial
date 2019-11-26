#include "Shared.hlsli"

static const float3 KFresnel_dielectric = float3(0.04, 0.04, 0.04); // 0.04 is simplified dielectric F0 value
static const float3 KMonochromatic = float3(0.2126, 0.7152, 0.0722);

// << Diffuse BRDF >>
//           Albedo
// BRDF() = --------
//             ¥ð
static float3 DiffuseBRDF_Lambertian(float3 DiffuseColor)
{
	return DiffuseColor / KPI;
}

// Fresnel reflectance approximation by Schlick
static float3 Fresnel_Schlick(float3 F0, float MdotL)
{
	return F0 + (float3(1.0, 1.0, 1.0) - F0) * pow(1.0 - MdotL, 5.0);
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

// Monodirectional shadowing-masking function (S is either V or L)
static float G1_Smith_GGX(float NdotS, float Alpha)
{
	float AlphaSquare = Alpha * Alpha;

	float Numerator = 2 * NdotS;
	float Denominator = NdotS + sqrt(AlphaSquare + (1.0 - AlphaSquare) * NdotS * NdotS);
	return Numerator / Denominator;
}

// Bidirectional shadowing-masking function
static float G2_Smith_GGX(float NdotL, float NdotV, float Alpha)
{
	return G1_Smith_GGX(NdotL, Alpha) * G1_Smith_GGX(NdotV, Alpha);
}

// Isotropic normal distribution
static float NormalDistribution_GGX(float Alpha, float NdotM)
{
	float NdotHSquare = NdotM * NdotM;
	float AlphaSquare = Alpha * Alpha;

	float Numerator = AlphaSquare;
	float Denominator = NdotHSquare * (AlphaSquare - 1.0) + 1.0;
	Denominator = KPI * Denominator * Denominator;
	return Numerator / Denominator;
}

// << Specular BRDF >>
//                    F * G * D
// BRDF() = ----------------------------
//           4 * dot(N, Wi) * dot(N, Wo)
static float3 SpecularBRDF_GGX(float3 F0, float NdotL, float NdotV, float NdotM, float MdotL, float Roughness)
{
	float Alpha = max(Roughness * Roughness, 0.001);

	float3 F = Fresnel_Schlick(F0, MdotL);
	float G = G2_Smith_GGX(NdotL, NdotV, Alpha);
	float D = NormalDistribution_GGX(Alpha, NdotM);

	float3 Numerator = F * G * D;
	float Denominator = 4 * NdotL * NdotV;
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
	float NdotM = saturate(dot(H, Normal));
	float SpecularPower = 0;
	if (NdotM != 0) SpecularPower = pow(NdotM, SpecularExponent);
	float3 BlinnSpecular = SpecularColor * LightColor * SpecularPower * SpecularIntensity;

	float3 Result = PhongDiffuse + BlinnSpecular;
	return Result;
}