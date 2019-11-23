#include "Base.hlsli"
#include "BRDF.hlsli"

#define FLAG_ID_DIFFUSE 0x01
#define FLAG_ID_NORMAL 0x02
#define FLAG_ID_OPACITY 0x04
#define FLAG_ID_SPECULARINTENSITY 0x08
#define FLAG_ID_ROUGHNESS 0x10
#define FLAG_ID_METALNESS 0x20
#define FLAG_ID_AMBIENTOCCLUSION 0x40

#define FLAG_ID_ENVIRONMENT 0x4000
#define FLAG_ID_IRRADIANCE 0x8000

SamplerState CurrentSampler : register(s0);

Texture2D DiffuseTexture : register(t0);
Texture2D NormalTexture : register(t1);
Texture2D OpacityTexture : register(t2);
Texture2D SpecularIntensityTexture : register(t3);
Texture2D RoughnessTexture : register(t4);
Texture2D MetalnessTexture : register(t5);
Texture2D AmbientOcclusionTexture : register(t6);
// Displacement texture slot

TextureCube EnvironmentTexture : register(t50);
TextureCube IrradianceTexture : register(t51);

cbuffer cbFlags : register(b0)
{
	bool bUseTexture;
	bool bUseLighting;
	bool bUsePhysicallyBasedRendering;
	uint EnvironmentTextureMipLevels;
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
	uint	FlagsHasTexture;
	uint	FlagsIsTextureSRGB;
	float	Reserved;
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
	float Opacity = 1.0;
	float AmbientOcclusion = 1.0;
	
	if (bUseTexture == true)
	{
		if (FlagsHasTexture & FLAG_ID_DIFFUSE)
		{
			DiffuseColor = DiffuseTexture.Sample(CurrentSampler, Input.TexCoord.xy).xyz;

			// # Here we make sure that input RGB values are in linear-space!
			if (!(FlagsIsTextureSRGB & FLAG_ID_DIFFUSE))
			{
				// # Convert gamma-space RGB to linear-space RGB (sRGB)
				DiffuseColor = pow(DiffuseColor, 2.2);
			}
		}

		if (FlagsHasTexture & FLAG_ID_NORMAL)
		{
			WorldNormal = NormalTexture.Sample(CurrentSampler, Input.TexCoord.xy);
			WorldNormal = normalize((WorldNormal * 2.0f) - 1.0f);

			float3x3 TextureSpace = float3x3(Input.WorldTangent.xyz, Input.WorldBitangent.xyz, Input.WorldNormal.xyz);
			WorldNormal = normalize(float4(mul(WorldNormal.xyz, TextureSpace), 0.0f));
		}
		
		if (FlagsHasTexture & FLAG_ID_OPACITY)
		{
			float4 Sampled = OpacityTexture.Sample(CurrentSampler, Input.TexCoord.xy);
			if (Sampled.r == Sampled.g && Sampled.g == Sampled.b)
			{
				Opacity = Sampled.r;
			}
			else
			{
				Opacity = Sampled.a;
			}
		}

		if (FlagsHasTexture & FLAG_ID_SPECULARINTENSITY)
		{
			SpecularIntensity = SpecularIntensityTexture.Sample(CurrentSampler, Input.TexCoord.xy).r;
		}

		if (FlagsHasTexture & FLAG_ID_ROUGHNESS)
		{
			Roughness = RoughnessTexture.Sample(CurrentSampler, Input.TexCoord.xy).r;
		}

		if (FlagsHasTexture & FLAG_ID_METALNESS)
		{
			Metalness = MetalnessTexture.Sample(CurrentSampler, Input.TexCoord.xy).r;
		}

		if (FlagsHasTexture & FLAG_ID_AMBIENTOCCLUSION)
		{
			AmbientOcclusion = AmbientOcclusionTexture.Sample(CurrentSampler, Input.TexCoord.xy).r;
		}
	}

	if (FlagsHasTexture & FLAG_ID_DIFFUSE)
	{
		AmbientColor = SpecularColor = DiffuseColor;
	}

	float3 MacrosurfaceNormal = WorldNormal.xyz;
	float3 Albedo = DiffuseColor;
	float4 OutputColor = float4(Albedo, 1);
	if (bUseLighting == true)
	{
		// Exposure tone mapping for raw albedo
		Albedo = float3(1.0, 1.0, 1.0) - exp(-Albedo * Exposure);

		float3 N = MacrosurfaceNormal; // Macrosurface normal vector

		// This is equivalent of L vector (light direction from a point on interface-- both for macrosurface and microsurface.)
		float3 Wi_direct = DirectionalLightDirection.xyz;

		// This is equivalent of V vector (view direction from a point on interface-- both for macrosurface and microsurface.)
		float3 Wo = normalize(EyePosition.xyz - Input.WorldPosition.xyz); 

		if (bUsePhysicallyBasedRendering == true)
		{
			// Calculate Fresnel reflectance at incident angle of 0 degree
			float3 F0 = lerp(KFresnel_dielectric, Albedo, Metalness);

			// This is equivalent of M vector == Half-way direction between Wi(== L) and Wo(== V)
			// Which is also visible microsurfaces' normal vector
			float3 M = normalize(Wi_direct + Wo);

			float NdotWi_direct = max(dot(N, Wi_direct), 0.001); // NaN
			float MdotWi_direct = max(dot(M, Wi_direct), 0);
			float NdotWo = max(dot(N, Wo), 0.001); // NaN
			float NdotM = max(dot(N, M), 0);

			// Direct light
			{
				// Radiance of direct light
				float3 Li_direct = DirectionalLightColor;

				float3 DiffuseBRDF = DiffuseBRDF_Lambertian(Albedo);
				float3 SpecularBRDF = SpecularBRDF_GGX(F0, NdotWi_direct, NdotWo, NdotM, MdotWi_direct, Roughness);

				// Calculate Fresnel reflectance of macrosurface
				float3 F_Macrosurface_direct = Fresnel_Schlick(F0, NdotWi_direct);

				// Specular light intensity
				float Ks_direct = dot(KMonochromatic, F_Macrosurface_direct); // Monochromatic intensity calculation

				// Diffuse light intensity
				float Kd_direct = 1.0 - Ks_direct;

				// (Outgoing) Radiance of direct light
				// L_o = ∫(BRDF() * L_i * cosθ)dω  <= Reflectance equation <= Rendering equation
				
				float3 Lo_direct_diff = (Kd_direct * DiffuseBRDF) * Li_direct * NdotWi_direct;
				float3 Lo_direct_spec = (Ks_direct * SpecularBRDF) * Li_direct * NdotWi_direct;

				OutputColor.xyz = Lo_direct_diff + Lo_direct_spec;
			}

			// Indirect light
			if (EnvironmentTextureMipLevels > 0)
			{
				// Indirect light direction
				float3 Wi_indirect = normalize(-Wo + (2.0 * dot(N, Wo) * N)); // @important: do not clamp dot product...!!!!
				float NdotWi_indirect = max(dot(N, Wi_indirect), 0.001); // NaN

				float3 M = normalize(Wi_indirect + Wo);
				float MdotWi_indirect = max(dot(M, Wi_indirect), 0);

				// Diffuse: Irradiance of the surface point
				float3 Ei_indirect = IrradianceTexture.SampleBias(CurrentSampler, N, Roughness * EnvironmentTextureMipLevels).rgb;
				if (!(FlagsIsTextureSRGB & FLAG_ID_IRRADIANCE)) Ei_indirect = pow(Ei_indirect, 2.2);

				// Specular: Radiance of indirect light
				float3 Li_indirect = EnvironmentTexture.SampleBias(CurrentSampler, Wi_indirect, Roughness * EnvironmentTextureMipLevels).rgb;
				if (!(FlagsIsTextureSRGB & FLAG_ID_ENVIRONMENT)) Li_indirect = pow(Li_indirect, 2.2);

				// Calculate Fresnel reflectance of macrosurface
				//float3 F_Macrosurface_indirect = Fresnel_Schlick(F0, NdotWi_indirect);
				float Ks_indirect = Metalness;
				float Kd_indirect = 1.0 - Ks_indirect;

				//float3 SpecularBRDF_indirect = SpecularBRDF_GGX(F0, NdotWi_indirect, NdotWo, NdotM, MdotWi_indirect, Roughness);

				float3 Lo_indirect_diff = Kd_indirect * Ei_indirect * Albedo / KPI;
				float3 Lo_indirect_spec = Ks_indirect * Li_indirect * Albedo; // @important: PSEUDO-specular

				OutputColor.xyz += (Lo_indirect_diff + Lo_indirect_spec) * AmbientOcclusion;
			}
		}
		else
		{
			float3 Ambient = CalculateClassicalAmbient(AmbientColor, AmbientLightColor, AmbientLightIntensity);
			float3 Directional = CalculateClassicalDirectional(Albedo, SpecularColor, MaterialSpecularExponent, SpecularIntensity,
				DirectionalLightColor, Wi_direct, Wo, N);

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
	if (FlagsHasTexture & FLAG_ID_OPACITY) OutputColor.a *= Opacity;
	
	return OutputColor;
}