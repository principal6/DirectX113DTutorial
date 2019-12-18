#include "Terrain.hlsli"
#include "BRDF.hlsli"
#include "GBuffer.hlsli"
#include "iPSCBs.hlsli"

cbuffer cbMaskingSpace : register(b3)
{
	float4x4 MaskingSpaceMatrix;
}

cbuffer cbSelection : register(b4)
{
	bool bShowSelection;
	float SelectionRadius;
	float2 AnaloguePosition;

	float4x4 TerrainWorld;
	float4x4 InverseTerrainWorld;
}

cbuffer cbEditorTime : register(b5)
{
	float NormalizedTime;
	float NormalizedTimeHalfSpeed;
	float2 Pad2;
}

#define FLAG_ID_DIFFUSE 0x01
#define FLAG_ID_NORMAL 0x02
#define FLAG_ID_OPACITY 0x04
#define FLAG_ID_SPECULARINTENSITY 0x08
#define FLAG_ID_ROUGHNESS 0x10
#define FLAG_ID_METALNESS 0x20
#define FLAG_ID_AMBIENTOCCLUSION 0x40

#define FLAG_ID_ENVIRONMENT 0x4000
#define FLAG_ID_IRRADIANCE 0x8000

SamplerState LinearWrapSampler : register(s0);
SamplerState LinearClampSampler : register(s1);

Texture2D Layer0DiffuseTexture : register(t0);
Texture2D Layer0NormalTexture : register(t1);
// Opacity texture slot
Texture2D Layer0SpecularIntensityTexture : register(t3);
Texture2D Layer0RoughnessTexture : register(t4);
// Metalness texture slot
Texture2D Layer0AmbientOcclusionTexture : register(t6);
// Displacement texture slot

Texture2D Layer1DiffuseTexture : register(t8);
Texture2D Layer1NormalTexture : register(t9);
// Opacity texture slot
Texture2D Layer1SpecularIntensityTexture : register(t11);
Texture2D Layer1RoughnessTexture : register(t12);
// Metalness texture slot
Texture2D Layer1AmbientOcclusionTexture : register(t14);
// Displacement texture slot

Texture2D Layer2DiffuseTexture : register(t16);
Texture2D Layer2NormalTexture : register(t17);
// Opacity texture slot
Texture2D Layer2SpecularIntensityTexture : register(t19);
Texture2D Layer2RoughnessTexture : register(t20);
// Metalness texture slot
Texture2D Layer2AmbientOcclusionTexture : register(t22);
// Displacement texture slot

Texture2D Layer3DiffuseTexture : register(t24);
Texture2D Layer3NormalTexture : register(t25);
// Opacity texture slot
Texture2D Layer3SpecularIntensityTexture : register(t27);
Texture2D Layer3RoughnessTexture : register(t28);
// Metalness texture slot
Texture2D Layer3AmbientOcclusionTexture : register(t30);
// Displacement texture slot

Texture2D MaskingTexture : register(t40);

TextureCube EnvironmentTexture : register(t50);
TextureCube IrradianceTexture : register(t51);
TextureCube PrefilteredRadianceTexture : register(t52);
Texture2D IntegratedBRDFTexture : register(t53);

#define TEX_COORD Input.TexCoord.xy
#define N BlendedNormal.xyz // Macrosurface normal

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float4 WorldVertexPosition = float4(Input.WorldPosition.x, 0, -Input.WorldPosition.z, 1);
	float4 LocalMaskingPosition = mul(WorldVertexPosition, InverseTerrainWorld);
	float4 MaskingSpacePosition = mul(LocalMaskingPosition, MaskingSpaceMatrix);
	float4 Masking = MaskingTexture.Sample(LinearWrapSampler, MaskingSpacePosition.xz);

	float4 BlendedDiffuse = float4(MaterialDiffuseColor, 1);
	if (FlagsHasTexture & FLAG_ID_DIFFUSE)
	{
		float4 DiffuseLayer0 = (TotalMaterialCount >= 1) ? Layer0DiffuseTexture.Sample(LinearWrapSampler, TEX_COORD) : float4(1, 1, 1, 1);
		float4 DiffuseLayer1 = (TotalMaterialCount >= 2) ? Layer1DiffuseTexture.Sample(LinearWrapSampler, TEX_COORD) : float4(1, 1, 1, 1);
		float4 DiffuseLayer2 = (TotalMaterialCount >= 3) ? Layer2DiffuseTexture.Sample(LinearWrapSampler, TEX_COORD) : float4(1, 1, 1, 1);
		float4 DiffuseLayer3 = (TotalMaterialCount >= 4) ? Layer3DiffuseTexture.Sample(LinearWrapSampler, TEX_COORD) : float4(1, 1, 1, 1);

		// # Here we make sure that input RGB values are in linear-space!
		// # Convert gamma-space RGB to linear-space RGB
		if (!(FlagsIsTextureSRGB & FLAG_ID_DIFFUSE))
		{
			if (TotalMaterialCount >= 1) DiffuseLayer0.xyz = pow(DiffuseLayer0.xyz, 2.2);
			if (TotalMaterialCount >= 2) DiffuseLayer1.xyz = pow(DiffuseLayer1.xyz, 2.2);
			if (TotalMaterialCount >= 3) DiffuseLayer2.xyz = pow(DiffuseLayer2.xyz, 2.2);
			if (TotalMaterialCount >= 4) DiffuseLayer3.xyz = pow(DiffuseLayer3.xyz, 2.2);
		}

		if (TotalMaterialCount >= 1) BlendedDiffuse = DiffuseLayer0;
		if (TotalMaterialCount >= 2) BlendedDiffuse.xyz = lerp(BlendedDiffuse.xyz, DiffuseLayer1.xyz, Masking.r);
		if (TotalMaterialCount >= 3) BlendedDiffuse.xyz = lerp(BlendedDiffuse.xyz, DiffuseLayer2.xyz, Masking.g);
		if (TotalMaterialCount >= 4) BlendedDiffuse.xyz = lerp(BlendedDiffuse.xyz, DiffuseLayer3.xyz, Masking.b);
	}
	
	float4 BlendedNormal = Input.WorldNormal;
	if (FlagsHasTexture & FLAG_ID_NORMAL)
	{
		float4 NormalLayer0 = (TotalMaterialCount >= 1) ? normalize((Layer0NormalTexture.Sample(LinearWrapSampler, TEX_COORD) * 2.0f) - 1.0f) : float4(0, 1, 0, 1);
		float4 NormalLayer1 = (TotalMaterialCount >= 2) ? normalize((Layer1NormalTexture.Sample(LinearWrapSampler, TEX_COORD) * 2.0f) - 1.0f) : float4(0, 1, 0, 1);
		float4 NormalLayer2 = (TotalMaterialCount >= 3) ? normalize((Layer2NormalTexture.Sample(LinearWrapSampler, TEX_COORD) * 2.0f) - 1.0f) : float4(0, 1, 0, 1);
		float4 NormalLayer3 = (TotalMaterialCount >= 4) ? normalize((Layer3NormalTexture.Sample(LinearWrapSampler, TEX_COORD) * 2.0f) - 1.0f) : float4(0, 1, 0, 1);

		if (TotalMaterialCount >= 1) BlendedNormal = NormalLayer0;
		if (TotalMaterialCount >= 2) BlendedNormal.xyz = lerp(BlendedNormal.xyz, NormalLayer1.xyz, Masking.r);
		if (TotalMaterialCount >= 3) BlendedNormal.xyz = lerp(BlendedNormal.xyz, NormalLayer2.xyz, Masking.g);
		if (TotalMaterialCount >= 4) BlendedNormal.xyz = lerp(BlendedNormal.xyz, NormalLayer3.xyz, Masking.b);

		const float3x3 KTextureSpace = float3x3(Input.WorldTangent.xyz, Input.WorldBitangent.xyz, Input.WorldNormal.xyz);
		BlendedNormal = normalize(BlendedNormal);
		BlendedNormal = normalize(float4(mul(BlendedNormal.xyz, KTextureSpace), 0.0f));
	}

	float BlendedSpecularIntensity = MaterialSpecularIntensity;
	if (FlagsHasTexture & FLAG_ID_SPECULARINTENSITY)
	{
		float SpecularIntensityLayer0 = (TotalMaterialCount >= 1) ? Layer0SpecularIntensityTexture.Sample(LinearWrapSampler, TEX_COORD).r : 0;
		float SpecularIntensityLayer1 = (TotalMaterialCount >= 2) ? Layer1SpecularIntensityTexture.Sample(LinearWrapSampler, TEX_COORD).r : 0;
		float SpecularIntensityLayer2 = (TotalMaterialCount >= 3) ? Layer2SpecularIntensityTexture.Sample(LinearWrapSampler, TEX_COORD).r : 0;
		float SpecularIntensityLayer3 = (TotalMaterialCount >= 4) ? Layer3SpecularIntensityTexture.Sample(LinearWrapSampler, TEX_COORD).r : 0;

		if (TotalMaterialCount >= 1) BlendedSpecularIntensity = SpecularIntensityLayer0;
		if (TotalMaterialCount >= 2) BlendedSpecularIntensity = lerp(BlendedSpecularIntensity, SpecularIntensityLayer1, Masking.r);
		if (TotalMaterialCount >= 3) BlendedSpecularIntensity = lerp(BlendedSpecularIntensity, SpecularIntensityLayer2, Masking.g);
		if (TotalMaterialCount >= 4) BlendedSpecularIntensity = lerp(BlendedSpecularIntensity, SpecularIntensityLayer3, Masking.b);
	}

	float BlendedRoughness = MaterialRoughness;
	if (FlagsHasTexture & FLAG_ID_ROUGHNESS)
	{
		float RoughnessLayer0 = (TotalMaterialCount >= 1) ? Layer0RoughnessTexture.Sample(LinearWrapSampler, TEX_COORD).r : 0;
		float RoughnessLayer1 = (TotalMaterialCount >= 2) ? Layer1RoughnessTexture.Sample(LinearWrapSampler, TEX_COORD).r : 0;
		float RoughnessLayer2 = (TotalMaterialCount >= 3) ? Layer2RoughnessTexture.Sample(LinearWrapSampler, TEX_COORD).r : 0;
		float RoughnessLayer3 = (TotalMaterialCount >= 4) ? Layer3RoughnessTexture.Sample(LinearWrapSampler, TEX_COORD).r : 0;

		if (TotalMaterialCount >= 1) BlendedRoughness = RoughnessLayer0;
		if (TotalMaterialCount >= 2) BlendedRoughness = lerp(BlendedRoughness, RoughnessLayer1, Masking.r);
		if (TotalMaterialCount >= 3) BlendedRoughness = lerp(BlendedRoughness, RoughnessLayer2, Masking.g);
		if (TotalMaterialCount >= 4) BlendedRoughness = lerp(BlendedRoughness, RoughnessLayer3, Masking.b);
	}

	float BlendedAmbientOcclusion = 1.0;
	if (FlagsHasTexture & FLAG_ID_METALNESS)
	{
		float AmbientOcclusionLayer0 = (TotalMaterialCount >= 1) ? Layer0AmbientOcclusionTexture.Sample(LinearWrapSampler, TEX_COORD).r : 1;
		float AmbientOcclusionLayer1 = (TotalMaterialCount >= 2) ? Layer1AmbientOcclusionTexture.Sample(LinearWrapSampler, TEX_COORD).r : 1;
		float AmbientOcclusionLayer2 = (TotalMaterialCount >= 3) ? Layer2AmbientOcclusionTexture.Sample(LinearWrapSampler, TEX_COORD).r : 1;
		float AmbientOcclusionLayer3 = (TotalMaterialCount >= 4) ? Layer3AmbientOcclusionTexture.Sample(LinearWrapSampler, TEX_COORD).r : 1;

		if (TotalMaterialCount >= 1) BlendedAmbientOcclusion = AmbientOcclusionLayer0;
		if (TotalMaterialCount >= 2) BlendedAmbientOcclusion = lerp(BlendedAmbientOcclusion, AmbientOcclusionLayer1, Masking.r);
		if (TotalMaterialCount >= 3) BlendedAmbientOcclusion = lerp(BlendedAmbientOcclusion, AmbientOcclusionLayer2, Masking.g);
		if (TotalMaterialCount >= 4) BlendedAmbientOcclusion = lerp(BlendedAmbientOcclusion, AmbientOcclusionLayer3, Masking.b);
	}
	
	// Selection highlight (for edit mode)
	bool bIsHighlitPixel = false;
	float4 HighlitAlbedo = BlendedDiffuse;
	if (bShowSelection == true)
	{
		const float3 ColorCmp = float3(0.2f, 0.4f, 0.6f);
		const float3 HighlightFactor = float3(0.2f, 0.2f, 0.2f);
		const float Sine = sin(NormalizedTimeHalfSpeed * KPI);
		
		float4 TerrainPosition = float4(Input.WorldPosition.x, 0, Input.WorldPosition.z, 1);
		float4 WorldSelectionPosition = float4(AnaloguePosition.x, 0, AnaloguePosition.y, 1);
		float Distance = distance(TerrainPosition, WorldSelectionPosition);
		if (Distance <= SelectionRadius)
		{
			BlendedDiffuse.xyz = max(BlendedDiffuse.xyz, ColorCmp);
			BlendedDiffuse.xyz += HighlightFactor * Sine;
			bIsHighlitPixel = true;
			HighlitAlbedo = BlendedDiffuse;
		}
	}

	float4 OutputColor = float4(BlendedDiffuse.xyz, 1);
	{
		// Exposure tone mapping for raw albedo
		float3 Albedo = float3(1.0, 1.0, 1.0) - exp(-BlendedDiffuse.xyz * Exposure);

		// This is equivalent of L vector (light direction from a point on interface-- both for macrosurface and microsurface.)
		float3 Wi_direct = DirectionalLightDirection.xyz;

		// This is equivalent of V vector (view direction from a point on interface-- both for macrosurface and microsurface.)
		float3 Wo = normalize(EyePosition.xyz - Input.WorldPosition.xyz);

		{
			// Calculate Fresnel reflectance at incident angle of 0 degree
			float3 F0 = lerp(KFresnel_dielectric, Albedo, MaterialMetalness);

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
				float3 SpecularBRDF = SpecularBRDF_GGX(F0, NdotWi_direct, NdotWo, NdotM, MdotWi_direct, BlendedRoughness);

				// Calculate Fresnel reflectance of macrosurface
				float3 F_Macrosurface_direct = Fresnel_Schlick(F0, NdotWi_direct);

				// Specular light intensity
				float Ks_direct = dot(KMonochromatic, F_Macrosurface_direct); // Monochromatic intensity calculation

				// Diffuse light intensity
				float Kd_direct = 1.0 - Ks_direct;

				// (Outgoing) Radiance of direct light
				// L_o = ¡ò(BRDF() * L_i * cos¥è)d¥ø  <= Reflectance equation <= Rendering equation

				float3 Lo_direct_diff = (Kd_direct * DiffuseBRDF) * Li_direct * NdotWi_direct;
				float3 Lo_direct_spec = (Ks_direct * SpecularBRDF) * Li_direct * NdotWi_direct;

				OutputColor.xyz = Lo_direct_diff + Lo_direct_spec;
			}

			// @important: below codes are commented because of the performance issue in my poor laptop...
			// Indirect light
			{
				OutputColor.xyz += CalculateClassicalAmbient(Albedo, AmbientLightColor, AmbientLightIntensity);

				/*
				// Diffuse: Irradiance of the surface point
				float3 Ei_indirect = IrradianceTexture.SampleBias(LinearWrapSampler, N, BlendedRoughness * (float)(IrradianceTextureMipLevels - 1)).rgb;

				OutputColor.xyz += Ei_indirect * Albedo * K1DIVPI * BlendedAmbientOcclusion;
				*/
			}
		}
	}

	// # Here we make sure that output RGB values are in gamma-space!
	// # Convert linear-space RGB to gamma-space RGB
	OutputColor.xyz = pow(OutputColor.xyz, 0.4545);

	// For normal & tangent drawing
	if (Input.bUseVertexColor != 0) return Input.Color;
	if (bIsHighlitPixel == true) OutputColor = HighlitAlbedo;

	return OutputColor;
}

GBufferOutput gbuffer(VS_OUTPUT Input)
{
	float4 WorldVertexPosition = float4(Input.WorldPosition.x, 0, -Input.WorldPosition.z, 1);
	float4 LocalMaskingPosition = mul(WorldVertexPosition, InverseTerrainWorld);
	float4 MaskingSpacePosition = mul(LocalMaskingPosition, MaskingSpaceMatrix);
	float4 Masking = MaskingTexture.Sample(LinearWrapSampler, MaskingSpacePosition.xz);

	float4 BlendedDiffuse = float4(MaterialDiffuseColor, 1);
	if (FlagsHasTexture & FLAG_ID_DIFFUSE)
	{
		float4 DiffuseLayer0 = (TotalMaterialCount >= 1) ? Layer0DiffuseTexture.Sample(LinearWrapSampler, TEX_COORD) : float4(1, 1, 1, 1);
		float4 DiffuseLayer1 = (TotalMaterialCount >= 2) ? Layer1DiffuseTexture.Sample(LinearWrapSampler, TEX_COORD) : float4(1, 1, 1, 1);
		float4 DiffuseLayer2 = (TotalMaterialCount >= 3) ? Layer2DiffuseTexture.Sample(LinearWrapSampler, TEX_COORD) : float4(1, 1, 1, 1);
		float4 DiffuseLayer3 = (TotalMaterialCount >= 4) ? Layer3DiffuseTexture.Sample(LinearWrapSampler, TEX_COORD) : float4(1, 1, 1, 1);

		// # Here we make sure that input RGB values are in linear-space!
		// # Convert gamma-space RGB to linear-space RGB
		if (!(FlagsIsTextureSRGB & FLAG_ID_DIFFUSE))
		{
			if (TotalMaterialCount >= 1) DiffuseLayer0.xyz = pow(DiffuseLayer0.xyz, 2.2);
			if (TotalMaterialCount >= 2) DiffuseLayer1.xyz = pow(DiffuseLayer1.xyz, 2.2);
			if (TotalMaterialCount >= 3) DiffuseLayer2.xyz = pow(DiffuseLayer2.xyz, 2.2);
			if (TotalMaterialCount >= 4) DiffuseLayer3.xyz = pow(DiffuseLayer3.xyz, 2.2);
		}

		if (TotalMaterialCount >= 1) BlendedDiffuse = DiffuseLayer0;
		if (TotalMaterialCount >= 2) BlendedDiffuse.xyz = lerp(BlendedDiffuse.xyz, DiffuseLayer1.xyz, Masking.r);
		if (TotalMaterialCount >= 3) BlendedDiffuse.xyz = lerp(BlendedDiffuse.xyz, DiffuseLayer2.xyz, Masking.g);
		if (TotalMaterialCount >= 4) BlendedDiffuse.xyz = lerp(BlendedDiffuse.xyz, DiffuseLayer3.xyz, Masking.b);
	}

	float3 Normal = Input.WorldNormal.xyz;
	if (FlagsHasTexture & FLAG_ID_NORMAL)
	{
		const float3x3 KTextureSpace = float3x3(Input.WorldTangent.xyz, Input.WorldBitangent.xyz, Input.WorldNormal.xyz);
		Normal = normalize((Layer0NormalTexture.Sample(LinearWrapSampler, TEX_COORD).xyz * 2.0) - 1.0);
		Normal = normalize(mul(Normal, KTextureSpace));
	}

	float SpecularIntensity = (FlagsHasTexture & FLAG_ID_SPECULARINTENSITY) ? 
		Layer0SpecularIntensityTexture.Sample(LinearWrapSampler, TEX_COORD).r : MaterialSpecularIntensity;
	float Roughness = (FlagsHasTexture & FLAG_ID_ROUGHNESS) ? Layer0RoughnessTexture.Sample(LinearWrapSampler, TEX_COORD).r : MaterialRoughness;
	float AmbientOcclusion = (FlagsHasTexture & FLAG_ID_METALNESS) ? Layer0AmbientOcclusionTexture.Sample(LinearWrapSampler, TEX_COORD).r : 1.0;

	// Selection highlight (for edit mode)
	bool bIsHighlitPixel = false;
	float4 HighlitAlbedo = BlendedDiffuse;
	if (bShowSelection == true)
	{
		const float3 ColorCmp = float3(0.2f, 0.4f, 0.6f);
		const float3 HighlightFactor = float3(0.2f, 0.2f, 0.2f);
		const float Sine = sin(NormalizedTimeHalfSpeed * KPI);

		float4 TerrainPosition = float4(Input.WorldPosition.x, 0, Input.WorldPosition.z, 1);
		float4 WorldSelectionPosition = float4(AnaloguePosition.x, 0, AnaloguePosition.y, 1);
		float Distance = distance(TerrainPosition, WorldSelectionPosition);
		if (Distance <= SelectionRadius)
		{
			BlendedDiffuse.xyz = max(BlendedDiffuse.xyz, ColorCmp);
			BlendedDiffuse.xyz += HighlightFactor * Sine;
			bIsHighlitPixel = true;
			HighlitAlbedo = BlendedDiffuse;
		}
	}
	if (bIsHighlitPixel == true) BlendedDiffuse.xyz = HighlitAlbedo.xyz;

	GBufferOutput Output;
	Output.BaseColor_Rough = float4(BlendedDiffuse.xyz, Roughness);
	Output.Normal = float4(Normal * 0.5 + 0.5, 0);
	Output.MetalAO = float4(0.5, AmbientOcclusion, 0, 0); // Metalness 0.5 is Terrain signature
	return Output;
}
