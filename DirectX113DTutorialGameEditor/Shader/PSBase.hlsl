#include "Base.hlsli"
#include "BRDF.hlsli"

SamplerState CurrentSampler : register(s0);
Texture2D DiffuseTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D OpacityTexture : register(t2);
Texture2D SpecularIntensityTexture : register(t3);
Texture2D RoughnessTexture : register(t4);
Texture2D MetalnessTexture : register(t5);
// Displacement texture slot

cbuffer cbFlags : register(b0)
{
	bool bUseTexture;
	bool bUseLighting;
	bool bUsePhysicallyBasedRendering;
	bool ReservedFlag;
}

cbuffer cbLight : register(b1)
{
	float4	DirectionalLightDirection;
	float3	DirectionalLightColor;
	float	Exposure;
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
	float	MaterialRoughness;

	float	MaterialMetalness;
	bool	bHasDiffuseTexture;
	bool	bHasNormalTexture;
	bool	bHasOpacityTexture;

	bool	bHasSpecularIntensityTexture;
	bool	bHasRoughnessTexture;
	bool	bHasMetalnessTexture;
	bool	Reserved;
}

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float3 AmbientColor = MaterialAmbientColor;
	float3 DiffuseColor = MaterialDiffuseColor;
	float3 SpecularColor = MaterialSpecularColor;
	float4 WorldNormal = normalize(Input.WorldNormal);
	float SpecularIntensity = MaterialSpecularIntensity;
	float Roughness = MaterialRoughness;
	float Metalness = MaterialMetalness;
	float Opacity = 1.0f;
	
	if (bUseTexture == true)
	{
		if (bHasDiffuseTexture == true)
		{
			AmbientColor = DiffuseColor = SpecularColor = DiffuseTexture.Sample(CurrentSampler, Input.UV.xy).xyz;
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

		if (bHasRoughnessTexture == true)
		{
			Roughness = RoughnessTexture.Sample(CurrentSampler, Input.UV.xy).r;
		}

		if (bHasMetalnessTexture == true)
		{
			Metalness = MetalnessTexture.Sample(CurrentSampler, Input.UV.xy).r;
		}
	}

	// # Here we make sure that input RGB values are in linear-space!
	// (In this project, all textures are loaded with their values in gamma-space)
	if (bHasDiffuseTexture == true)
	{
		// # Convert gamma-space RGB to linear-space RGB (sRGB)
		DiffuseColor = pow(DiffuseColor, 2.2);
		AmbientColor = pow(AmbientColor, 2.2);
		SpecularColor = pow(SpecularColor, 2.2);
	}

	float3 MacrosurfaceNormal = WorldNormal.xyz;
	float3 BaseColor = DiffuseColor;
	float4 OutputColor = float4(BaseColor, 1);
	if (bUseLighting == true)
	{
		// Exposure tone mapping
		BaseColor = float3(1.0, 1.0, 1.0) - exp(-BaseColor * Exposure);

		float3 N = MacrosurfaceNormal;
		float3 L = DirectionalLightDirection.xyz;
		float3 V = normalize(EyePosition.xyz - Input.WorldPosition.xyz);

		if (bUsePhysicallyBasedRendering == true)
		{
			// Radiance for direct light
			float3 Li = DirectionalLightColor;

			float3 H = normalize(L + V);
			float NdotL = max(dot(N, L), 0.0001); // NaN
			float NdotV = max(dot(N, V), 0.0001); // NaN
			float NdotH = max(dot(N, H), 0.0001); // NaN
			float HdotL = max(dot(L, H), 0);

			// ### PBR direct light
			// Calculate Fresnel reflectance at incident angle 0
			float3 F0 = lerp(KF_Dielectric, BaseColor, Metalness);

			// Calculate Fresnel reflectance of macrosurface
			float3 F_Macrosurface = Fresnel_Schlick(F0, NdotL);

			// Specular light intensity
			float Ks = dot(KMonochromatic, F_Macrosurface); // Monochromatic intensity calculation
			
			// Diffuse light intensity
			float Kd = 1.0 - Ks;

			float3 DiffuseBRDF = DiffuseBRDF_Lambertian(BaseColor);
			float3 SpecularBRDF = SpecularBRDF_GGX(F0, NdotL, NdotV, NdotH, HdotL, Roughness);

			// ### Still non-PBR indirect light
			float3 Ambient = CalculateClassicalAmbient(F0, AmbientLightColor, AmbientLightIntensity);
			float Ka_NonPBR = 0.03;
			
			OutputColor.xyz = Ambient * Ka_NonPBR + (1.0 - Ka_NonPBR) * (NdotL * Li * (Kd * DiffuseBRDF + Ks * SpecularBRDF));
		}
		else
		{
			float3 Ambient = CalculateClassicalAmbient(AmbientColor, AmbientLightColor, AmbientLightIntensity);
			float3 Directional = CalculateClassicalDirectional(BaseColor, SpecularColor, MaterialSpecularExponent, SpecularIntensity,
				DirectionalLightColor, L, V, N);

			// Directional Light의 위치가 지평선에 가까워질수록 빛의 세기를 약하게 한다.
			float Dot = dot(DirectionalLightDirection, KUpDirection);
			Directional.xyz *= pow(Dot, 0.6f);

			OutputColor.xyz = Ambient + Directional;
		}
	}

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB (sRGB) to gamma-space RGB
	OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	if (Input.bUseVertexColor != 0) OutputColor = Input.Color;
	if (bHasOpacityTexture == true) OutputColor.a *= Opacity;
	
	return OutputColor;
}