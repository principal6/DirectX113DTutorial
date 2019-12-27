#include "Base.hlsli"
#include "BRDF.hlsli"
#include "GBuffer.hlsli"
#include "iPSCBs.hlsli"

cbuffer cbSceneMaterial : register(b3)
{
	uint FlagsHasSceneTexture;
	uint FlagsIsSceneTextureSRGB;
	float2 Reserved2;
}

#define FLAG_ID_DIFFUSE 0x01
#define FLAG_ID_NORMAL 0x02
#define FLAG_ID_OPACITY 0x04
#define FLAG_ID_SPECULARINTENSITY 0x08
#define FLAG_ID_ROUGHNESS 0x10
#define FLAG_ID_METALNESS 0x20
#define FLAG_ID_AMBIENTOCCLUSION 0x40

#define FLAG_ID_IGNORE_SCENE_MATERIAL 0x2000

#define FLAG_ID_ENVIRONMENT 0x4000
#define FLAG_ID_IRRADIANCE 0x8000

SamplerState LinearWrapSampler : register(s0);
SamplerState LinearClampSampler : register(s1);

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
TextureCube PrefilteredRadianceTexture : register(t52);
Texture2D IntegratedBRDFTexture : register(t53);

Texture2D SceneDiffuseTexture : register(t60);
Texture2D SceneNormalTexture : register(t61);
Texture2D SceneOpacityTexture : register(t62);
Texture2D SceneRoughnessTexture : register(t63);
Texture2D SceneMetalnessTexture : register(t64);
Texture2D SceneAmbientOcclusionTexture : register(t65);

#define N WorldNormal.xyz // Macrosurface normal

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	//float3 AmbientColor = MaterialAmbientColor;
	float3 DiffuseColor = MaterialDiffuseColor;
	//float3 SpecularColor = MaterialSpecularColor;
	float4 WorldNormal = normalize(Input.WorldNormal);
	//float SpecularIntensity = MaterialSpecularIntensity;
	float Roughness = MaterialRoughness;
	float Metalness = MaterialMetalness;
	float Opacity = 1.0;
	float AmbientOcclusion = 1.0;

	bool bShouldIgnoreSceneMaterial = FlagsHasTexture & FLAG_ID_IGNORE_SCENE_MATERIAL;
	
	if (FlagsHasTexture & FLAG_ID_DIFFUSE)
	{
		DiffuseColor = DiffuseTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).xyz;

		// # Here we make sure that input RGB values are in linear-space!
		if (!(FlagsIsTextureSRGB & FLAG_ID_DIFFUSE))
		{
			// # Convert gamma-space RGB to linear-space RGB
			DiffuseColor = pow(DiffuseColor, 2.2);
		}
	}
	else if (FlagsHasSceneTexture & FLAG_ID_DIFFUSE && !bShouldIgnoreSceneMaterial)
	{
		DiffuseColor = SceneDiffuseTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).xyz;

		if (!(FlagsIsSceneTextureSRGB & FLAG_ID_DIFFUSE))
		{
			DiffuseColor = pow(DiffuseColor, 2.2);
		}
	}

	if (FlagsHasTexture & FLAG_ID_NORMAL)
	{
		WorldNormal = NormalTexture.Sample(LinearWrapSampler, Input.TexCoord.xy);
		WorldNormal = normalize((WorldNormal * 2.0) - 1.0);
		float3x3 TextureSpace = float3x3(Input.WorldTangent.xyz, Input.WorldBitangent.xyz, Input.WorldNormal.xyz);
		WorldNormal = normalize(float4(mul(WorldNormal.xyz, TextureSpace), 0.0f));
	}
	else if (FlagsHasSceneTexture & FLAG_ID_NORMAL && !bShouldIgnoreSceneMaterial)
	{
		WorldNormal = SceneNormalTexture.Sample(LinearWrapSampler, Input.TexCoord.xy);
		WorldNormal = normalize((WorldNormal * 2.0) - 1.0);
		float3x3 TextureSpace = float3x3(Input.WorldTangent.xyz, Input.WorldBitangent.xyz, Input.WorldNormal.xyz);
		WorldNormal = normalize(float4(mul(WorldNormal.xyz, TextureSpace), 0.0f));
	}

	if (FlagsHasTexture & FLAG_ID_OPACITY)
	{
		float4 Sampled = OpacityTexture.Sample(LinearWrapSampler, Input.TexCoord.xy);
		if (Sampled.r == Sampled.g && Sampled.g == Sampled.b)
		{
			Opacity = Sampled.r;
		}
		else
		{
			Opacity = Sampled.a;
		}
	}
	else if (FlagsHasSceneTexture & FLAG_ID_OPACITY && !bShouldIgnoreSceneMaterial)
	{
		float4 Sampled = SceneOpacityTexture.Sample(LinearWrapSampler, Input.TexCoord.xy);
		if (Sampled.r == Sampled.g && Sampled.g == Sampled.b)
		{
			Opacity = Sampled.r;
		}
		else
		{
			Opacity = Sampled.a;
		}
	}

	//if (FlagsHasTexture & FLAG_ID_SPECULARINTENSITY)
	//{
	//	SpecularIntensity = SpecularIntensityTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).r;
	//}

	if (FlagsHasTexture & FLAG_ID_ROUGHNESS)
	{
		Roughness = RoughnessTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).r;
	}
	else if (FlagsHasSceneTexture & FLAG_ID_ROUGHNESS && !bShouldIgnoreSceneMaterial)
	{
		Roughness = SceneRoughnessTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).r;
	}

	if (FlagsHasTexture & FLAG_ID_METALNESS)
	{
		Metalness = MetalnessTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).r;
	}
	else if (FlagsHasSceneTexture & FLAG_ID_METALNESS && !bShouldIgnoreSceneMaterial)
	{
		Metalness = SceneMetalnessTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).r;
	}

	if (FlagsHasTexture & FLAG_ID_AMBIENTOCCLUSION)
	{
		AmbientOcclusion = AmbientOcclusionTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).r;
	}
	else if (FlagsHasSceneTexture & FLAG_ID_AMBIENTOCCLUSION && !bShouldIgnoreSceneMaterial)
	{
		AmbientOcclusion = SceneAmbientOcclusionTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).r;
	}

	//if (FlagsHasTexture & FLAG_ID_DIFFUSE)
	//{
	//	AmbientColor = SpecularColor = DiffuseColor;
	//}

	float4 OutputColor = float4(DiffuseColor, 1);
	{
		// Exposure tone mapping for raw albedo
		float3 Albedo = float3(1.0, 1.0, 1.0) - exp(-DiffuseColor * Exposure);

		// This is equivalent of L vector (light direction from a point on the interface-- both for macrosurface and microsurface.)
		float3 Wi_direct = DirectionalLightDirection.xyz;

		// This is equivalent of V vector (view direction from a point on the interface-- both for macrosurface and microsurface.)
		float3 Wo = normalize(EyePosition.xyz - Input.WorldPosition.xyz); 

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
				// L_o = ��(BRDF() * L_i * cos��)d��  <= Reflectance equation <= Rendering equation
				
				float3 Lo_direct_diff = (Kd_direct * DiffuseBRDF) * Li_direct * NdotWi_direct;
				float3 Lo_direct_spec = (Ks_direct * SpecularBRDF) * Li_direct * NdotWi_direct;

				OutputColor.xyz = Lo_direct_diff + Lo_direct_spec;
			}

			// Indirect light
			if (IrradianceTextureMipLevels > 0)
			{
				// Indirect light direction
				float3 Wi_indirect = normalize(-Wo + (2.0 * dot(N, Wo) * N)); // @important: do not clamp dot product...!!!!
				float NdotWi_indirect = dot(N, Wi_indirect);

				// Diffuse: Irradiance of the surface point
				float3 Ei_indirect = IrradianceTexture.SampleBias(LinearWrapSampler, N, Roughness * (float)(IrradianceTextureMipLevels - 1)).rgb;
				if (!(FlagsIsTextureSRGB & FLAG_ID_IRRADIANCE)) Ei_indirect = pow(Ei_indirect, 2.2);

				// Calculate Fresnel reflectance of macrosurface
				float3 F_Macrosurface_indirect = Fresnel_Schlick(F0, NdotWi_indirect);

				// Specular light intensity
				float Ks_indirect = dot(KMonochromatic, F_Macrosurface_indirect); // Monochromatic intensity calculation
				float Kd_indirect = 1.0 - Ks_indirect;

				float3 Lo_indirect_diff = Kd_indirect * Ei_indirect * Albedo * K1DIVPI;

				//					1   N
				// L_prefiltered = ---  ��  Li cos��
				//					N  k=1
				// @important: use SampleLevel, not SampleBias (we must not depend on distance but on roughness when selecting mip level)
				float3 PrefilteredRadiance = PrefilteredRadianceTexture.SampleLevel(LinearWrapSampler, Wi_indirect,
					Roughness * (float)(PrefilteredRadianceTextureMipLevels - 1)).rgb;

				// @important: use linear clamp sampler in order to avoid sampling at border issues!!
				float2 IntegratedBRDF = IntegratedBRDFTexture.SampleLevel(LinearClampSampler, float2(saturate(dot(N, Wo)), 1 - Roughness), 0).rg;

				float3 Lo_indirect_spec = Ks_indirect * PrefilteredRadiance * (Albedo * IntegratedBRDF.x + IntegratedBRDF.y);

				OutputColor.xyz += (Lo_indirect_diff + Lo_indirect_spec) * AmbientOcclusion;
			}
		}
	}

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB to gamma-space RGB
	OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	if (Input.bUseVertexColor != 0) OutputColor = Input.Color;
	if (FlagsHasTexture & FLAG_ID_OPACITY) OutputColor.a *= Opacity;
	
	return OutputColor;
}

GBufferOutput GBuffer(VS_OUTPUT Input)
{
	float3 DiffuseColor = MaterialDiffuseColor;
	float4 WorldNormal = normalize(Input.WorldNormal);
	//float SpecularIntensity = MaterialSpecularIntensity;
	float Roughness = MaterialRoughness;
	float Metalness = MaterialMetalness;
	float AmbientOcclusion = 1.0;

	bool bShouldIgnoreSceneMaterial = FlagsHasTexture & FLAG_ID_IGNORE_SCENE_MATERIAL;

	if (FlagsHasTexture & FLAG_ID_DIFFUSE)
	{
		DiffuseColor = DiffuseTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).xyz;

		// # Here we make sure that input RGB values are in linear-space!
		if (!(FlagsIsTextureSRGB & FLAG_ID_DIFFUSE))
		{
			// # Convert gamma-space RGB to linear-space RGB
			DiffuseColor = pow(DiffuseColor, 2.2);
		}
	}
	else if (FlagsHasSceneTexture & FLAG_ID_DIFFUSE && !bShouldIgnoreSceneMaterial)
	{
		DiffuseColor = SceneDiffuseTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).xyz;
		if (!(FlagsIsSceneTextureSRGB & FLAG_ID_DIFFUSE))
		{
			DiffuseColor = pow(DiffuseColor, 2.2);
		}
	}

	if (FlagsHasTexture & FLAG_ID_NORMAL)
	{
		WorldNormal = NormalTexture.Sample(LinearWrapSampler, Input.TexCoord.xy);
		WorldNormal = normalize((WorldNormal * 2.0f) - 1.0f);
		float3x3 TextureSpace = float3x3(Input.WorldTangent.xyz, Input.WorldBitangent.xyz, Input.WorldNormal.xyz);
		WorldNormal = normalize(float4(mul(WorldNormal.xyz, TextureSpace), 0.0f));
	}
	else if (FlagsHasSceneTexture & FLAG_ID_NORMAL && !bShouldIgnoreSceneMaterial)
	{
		WorldNormal = SceneNormalTexture.Sample(LinearWrapSampler, Input.TexCoord.xy);
		WorldNormal = normalize((WorldNormal * 2.0f) - 1.0f);
		float3x3 TextureSpace = float3x3(Input.WorldTangent.xyz, Input.WorldBitangent.xyz, Input.WorldNormal.xyz);
		WorldNormal = normalize(float4(mul(WorldNormal.xyz, TextureSpace), 0.0f));
	}

	//if (FlagsHasTexture & FLAG_ID_SPECULARINTENSITY) SpecularIntensity = SpecularIntensityTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).r;
	
	if (FlagsHasTexture & FLAG_ID_ROUGHNESS)
	{
		Roughness = RoughnessTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).r;
	}
	else if (FlagsHasSceneTexture & FLAG_ID_ROUGHNESS && !bShouldIgnoreSceneMaterial)
	{
		Roughness = SceneRoughnessTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).r;
	}

	if (FlagsHasTexture & FLAG_ID_METALNESS)
	{
		Metalness = MetalnessTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).r;
	}
	else if (FlagsHasSceneTexture & FLAG_ID_METALNESS && !bShouldIgnoreSceneMaterial)
	{
		Metalness = SceneMetalnessTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).r;
	}
	
	if (FlagsHasTexture & FLAG_ID_AMBIENTOCCLUSION)
	{
		AmbientOcclusion = AmbientOcclusionTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).r;
	}
	else if (FlagsHasSceneTexture & FLAG_ID_AMBIENTOCCLUSION && !bShouldIgnoreSceneMaterial)
	{
		AmbientOcclusion = SceneAmbientOcclusionTexture.Sample(LinearWrapSampler, Input.TexCoord.xy).r;
	}

	float3 BaseColor = float3(1.0, 1.0, 1.0) - exp(-DiffuseColor * Exposure);
	float NormalA = 0;
	if (Input.bUseVertexColor != 0)
	{
		BaseColor = Input.Color.xyz;
		NormalA = 1;
	}
	GBufferOutput Output;
	Output.BaseColor_Rough = float4(BaseColor, Roughness);
	Output.Normal = float4(N * 0.5 + 0.5, NormalA);
	Output.MetalAO = float4(Metalness, AmbientOcclusion, 0, 0);
	return Output;
}

float4 Void() : SV_TARGET
{
	return float4(1, 1, 1, 1);
}

float4 RawVertexColor(VS_OUTPUT Input) : SV_TARGET
{
	float4 OutputColor = Input.Color;

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB to gamma-space RGB
	OutputColor.rgb = pow(OutputColor.rgb, 0.4545);

	return OutputColor;
}

float4 RawDiffuseColor() : SV_TARGET
{
	float4 OutputColor = float4(MaterialDiffuseColor, 1);

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB to gamma-space RGB
	OutputColor.rgb = pow(OutputColor.rgb, 0.4545);

	return OutputColor;
}