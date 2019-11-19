#include "Terrain.hlsli"
#include "BRDF.hlsli"

#define FLAG_ID_DIFFUSE 0x01
#define FLAG_ID_NORMAL 0x02
#define FLAG_ID_OPACITY 0x04
#define FLAG_ID_SPECULARINTENSITY 0x08
#define FLAG_ID_ROUGHNESS 0x10
#define FLAG_ID_METALNESS 0x20
#define FLAG_ID_AMBIENTOCCLUSION 0x40

SamplerState TerrainSampler : register(s0);

Texture2D Layer0DiffuseTexture : register(t0);
Texture2D Layer0NormalTexture : register(t1);
Texture2D Layer0OpacityTexture : register(t2);
Texture2D Layer0SpecularIntensityTexture : register(t3);
Texture2D Layer0RoughnessTexture : register(t4);
Texture2D Layer0MetalnessTexture : register(t5);
Texture2D Layer0AmbientOcclusionTexture : register(t6);
// Displacement texture slot

Texture2D Layer1DiffuseTexture : register(t8);
Texture2D Layer1NormalTexture : register(t9);
Texture2D Layer1OpacityTexture : register(t10);
Texture2D Layer1SpecularIntensityTexture : register(t11);
Texture2D Layer1RoughnessTexture : register(t12);
Texture2D Layer1MetalnessTexture : register(t13);
Texture2D Layer1AmbientOcclusionTexture : register(t14);
// Displacement texture slot

Texture2D Layer2DiffuseTexture : register(t16);
Texture2D Layer2NormalTexture : register(t17);
Texture2D Layer2OpacityTexture : register(t18);
Texture2D Layer2SpecularIntensityTexture : register(t19);
Texture2D Layer2RoughnessTexture : register(t20);
Texture2D Layer2MetalnessTexture : register(t21);
Texture2D Layer2AmbientOcclusionTexture : register(t22);
// Displacement texture slot

Texture2D Layer3DiffuseTexture : register(t24);
Texture2D Layer3NormalTexture : register(t25);
Texture2D Layer3OpacityTexture : register(t26);
Texture2D Layer3SpecularIntensityTexture : register(t27);
Texture2D Layer3RoughnessTexture : register(t28);
Texture2D Layer3MetalnessTexture : register(t29);
Texture2D Layer3AmbientOcclusionTexture : register(t30);
// Displacement texture slot

Texture2D Layer4DiffuseTexture : register(t32);
Texture2D Layer4NormalTexture : register(t33);
Texture2D Layer4OpacityTexture : register(t34);
Texture2D Layer4SpecularIntensityTexture : register(t35);
Texture2D Layer4RoughnessTexture : register(t36);
Texture2D Layer4MetalnessTexture : register(t37);
Texture2D Layer4AmbientOcclusionTexture : register(t38);
// Displacement texture slot

Texture2D MaskingTexture : register(t40);

TextureCube EnvironmentTexture : register(t50);
TextureCube IrradianceTexture : register(t51);

cbuffer cbFlags : register(b0)
{
	bool bUseTexture;
	bool bUseLighting;
	bool bUsePhysicallyBasedRendering;
	uint EnvironmentTextureMipLevels;
}

cbuffer cbMaskingSpace : register(b1)
{
	float4x4 MaskingSpaceMatrix;
}

cbuffer cbLight : register(b2)
{
	float4	DirectionalLightDirection;
	float3	DirectionalLightColor;
	float	Exposure;
	float3	AmbientLightColor;
	float	AmbientLightIntensity;
	float4	EyePosition;
}

cbuffer cbSelection : register(b3)
{
	bool bShowSelection;
	float SelectionRadius;
	float2 AnaloguePosition;

	float4x4 TerrainWorld;
	float4x4 InverseTerrainWorld;
}

cbuffer cbEditorTime : register(b4)
{
	float NormalizedTime;
	float NormalizedTimeHalfSpeed;
	float2 Pad2;
}

cbuffer cbMaterial : register(b5)
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
	float4 WorldVertexPosition = float4(Input.WorldPosition.x, 0, -Input.WorldPosition.z, 1);
	float4 LocalMaskingPosition = mul(WorldVertexPosition, InverseTerrainWorld);
	float4 MaskingSpacePosition = mul(LocalMaskingPosition, MaskingSpaceMatrix);
	float4 Masking = MaskingTexture.Sample(TerrainSampler, MaskingSpacePosition.xz);

	const float2 KTexCoord = Input.TexCoord.xy;

	float4 DiffuseLayer0 = Layer0DiffuseTexture.Sample(TerrainSampler, KTexCoord);
	float4 DiffuseLayer1 = Layer1DiffuseTexture.Sample(TerrainSampler, KTexCoord);
	float4 DiffuseLayer2 = Layer2DiffuseTexture.Sample(TerrainSampler, KTexCoord);
	float4 DiffuseLayer3 = Layer3DiffuseTexture.Sample(TerrainSampler, KTexCoord);
	float4 DiffuseLayer4 = Layer4DiffuseTexture.Sample(TerrainSampler, KTexCoord);
	float4 BlendedDiffuse = float4(MaterialDiffuseColor, 1);
	if (FlagsHasTexture & FLAG_ID_DIFFUSE)
	{
		// # Here we make sure that input RGB values are in linear-space!
		if (!(FlagsIsTextureSRGB & FLAG_ID_DIFFUSE))
		{
			// # Convert gamma-space RGB to linear-space RGB (sRGB)
			DiffuseLayer0.xyz = pow(DiffuseLayer0.xyz, 2.2);
			DiffuseLayer1.xyz = pow(DiffuseLayer1.xyz, 2.2);
			DiffuseLayer2.xyz = pow(DiffuseLayer2.xyz, 2.2);
			DiffuseLayer3.xyz = pow(DiffuseLayer3.xyz, 2.2);
			DiffuseLayer4.xyz = pow(DiffuseLayer4.xyz, 2.2);
		}

		BlendedDiffuse = DiffuseLayer0;
		BlendedDiffuse.xyz = lerp(BlendedDiffuse.xyz, DiffuseLayer1.xyz, Masking.r);
		BlendedDiffuse.xyz = lerp(BlendedDiffuse.xyz, DiffuseLayer2.xyz, Masking.g);
		BlendedDiffuse.xyz = lerp(BlendedDiffuse.xyz, DiffuseLayer3.xyz, Masking.b);
		BlendedDiffuse.xyz = lerp(BlendedDiffuse.xyz, DiffuseLayer4.xyz, Masking.a);
	}
	
	float4 NormalLayer0 = normalize((Layer0NormalTexture.Sample(TerrainSampler, KTexCoord) * 2.0f) - 1.0f);
	float4 NormalLayer1 = normalize((Layer1NormalTexture.Sample(TerrainSampler, KTexCoord) * 2.0f) - 1.0f);
	float4 NormalLayer2 = normalize((Layer2NormalTexture.Sample(TerrainSampler, KTexCoord) * 2.0f) - 1.0f);
	float4 NormalLayer3 = normalize((Layer3NormalTexture.Sample(TerrainSampler, KTexCoord) * 2.0f) - 1.0f);
	float4 NormalLayer4 = normalize((Layer4NormalTexture.Sample(TerrainSampler, KTexCoord) * 2.0f) - 1.0f);
	float4 BlendedNormal = Input.WorldNormal;
	if (FlagsHasTexture & FLAG_ID_NORMAL)
	{
		const float3x3 KTextureSpace = float3x3(Input.WorldTangent.xyz, Input.WorldBitangent.xyz, Input.WorldNormal.xyz);
		BlendedNormal = NormalLayer0;
		BlendedNormal.xyz = lerp(BlendedNormal.xyz, NormalLayer1.xyz, Masking.r);
		BlendedNormal.xyz = lerp(BlendedNormal.xyz, NormalLayer2.xyz, Masking.g);
		BlendedNormal.xyz = lerp(BlendedNormal.xyz, NormalLayer3.xyz, Masking.b);
		BlendedNormal.xyz = lerp(BlendedNormal.xyz, NormalLayer4.xyz, Masking.a);
		BlendedNormal = normalize(BlendedNormal);
		BlendedNormal = normalize(float4(mul(BlendedNormal.xyz, KTextureSpace), 0.0f));
	}

	float SpecularIntensityLayer0 = Layer0SpecularIntensityTexture.Sample(TerrainSampler, KTexCoord).r;
	float SpecularIntensityLayer1 = Layer1SpecularIntensityTexture.Sample(TerrainSampler, KTexCoord).r;
	float SpecularIntensityLayer2 = Layer2SpecularIntensityTexture.Sample(TerrainSampler, KTexCoord).r;
	float SpecularIntensityLayer3 = Layer3SpecularIntensityTexture.Sample(TerrainSampler, KTexCoord).r;
	float SpecularIntensityLayer4 = Layer4SpecularIntensityTexture.Sample(TerrainSampler, KTexCoord).r;
	float BlendedSpecularIntensity = MaterialSpecularIntensity;
	if (FlagsHasTexture & FLAG_ID_SPECULARINTENSITY)
	{
		BlendedSpecularIntensity = SpecularIntensityLayer0;
		BlendedSpecularIntensity = lerp(BlendedSpecularIntensity, SpecularIntensityLayer1, Masking.r);
		BlendedSpecularIntensity = lerp(BlendedSpecularIntensity, SpecularIntensityLayer2, Masking.g);
		BlendedSpecularIntensity = lerp(BlendedSpecularIntensity, SpecularIntensityLayer3, Masking.b);
		BlendedSpecularIntensity = lerp(BlendedSpecularIntensity, SpecularIntensityLayer4, Masking.a);
	}

	float RoughnessLayer0 = Layer0RoughnessTexture.Sample(TerrainSampler, KTexCoord).r;
	float RoughnessLayer1 = Layer1RoughnessTexture.Sample(TerrainSampler, KTexCoord).r;
	float RoughnessLayer2 = Layer2RoughnessTexture.Sample(TerrainSampler, KTexCoord).r;
	float RoughnessLayer3 = Layer3RoughnessTexture.Sample(TerrainSampler, KTexCoord).r;
	float RoughnessLayer4 = Layer4RoughnessTexture.Sample(TerrainSampler, KTexCoord).r;
	float BlendedRoughness = MaterialRoughness;
	if (FlagsHasTexture & FLAG_ID_ROUGHNESS)
	{
		BlendedRoughness = RoughnessLayer0;
		BlendedRoughness = lerp(BlendedRoughness, RoughnessLayer1, Masking.r);
		BlendedRoughness = lerp(BlendedRoughness, RoughnessLayer2, Masking.g);
		BlendedRoughness = lerp(BlendedRoughness, RoughnessLayer3, Masking.b);
		BlendedRoughness = lerp(BlendedRoughness, RoughnessLayer4, Masking.a);
	}

	float MetalnessLayer0 = Layer0MetalnessTexture.Sample(TerrainSampler, KTexCoord).r;
	float MetalnessLayer1 = Layer1MetalnessTexture.Sample(TerrainSampler, KTexCoord).r;
	float MetalnessLayer2 = Layer2MetalnessTexture.Sample(TerrainSampler, KTexCoord).r;
	float MetalnessLayer3 = Layer3MetalnessTexture.Sample(TerrainSampler, KTexCoord).r;
	float MetalnessLayer4 = Layer4MetalnessTexture.Sample(TerrainSampler, KTexCoord).r;
	float BlendedMetalness = MaterialMetalness;
	if (FlagsHasTexture & FLAG_ID_METALNESS)
	{
		BlendedMetalness = MetalnessLayer0;
		BlendedMetalness = lerp(BlendedMetalness, MetalnessLayer1, Masking.r);
		BlendedMetalness = lerp(BlendedMetalness, MetalnessLayer2, Masking.g);
		BlendedMetalness = lerp(BlendedMetalness, MetalnessLayer3, Masking.b);
		BlendedMetalness = lerp(BlendedMetalness, MetalnessLayer4, Masking.a);
	}

	float AmbientOcclusionLayer0 = Layer0AmbientOcclusionTexture.Sample(TerrainSampler, KTexCoord).r;
	float AmbientOcclusionLayer1 = Layer1AmbientOcclusionTexture.Sample(TerrainSampler, KTexCoord).r;
	float AmbientOcclusionLayer2 = Layer2AmbientOcclusionTexture.Sample(TerrainSampler, KTexCoord).r;
	float AmbientOcclusionLayer3 = Layer3AmbientOcclusionTexture.Sample(TerrainSampler, KTexCoord).r;
	float AmbientOcclusionLayer4 = Layer4AmbientOcclusionTexture.Sample(TerrainSampler, KTexCoord).r;
	float BlendedAmbientOcclusion = 1.0;
	if (FlagsHasTexture & FLAG_ID_METALNESS)
	{
		BlendedAmbientOcclusion = AmbientOcclusionLayer0;
		BlendedAmbientOcclusion = lerp(BlendedAmbientOcclusion, AmbientOcclusionLayer1, Masking.r);
		BlendedAmbientOcclusion = lerp(BlendedAmbientOcclusion, AmbientOcclusionLayer2, Masking.g);
		BlendedAmbientOcclusion = lerp(BlendedAmbientOcclusion, AmbientOcclusionLayer3, Masking.b);
		BlendedAmbientOcclusion = lerp(BlendedAmbientOcclusion, AmbientOcclusionLayer4, Masking.a);
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

	float3 MacrosurfaceNormal = BlendedNormal.xyz;
	float3 Albedo = BlendedDiffuse.xyz;
	float4 OutputColor = float4(Albedo, 1);
	{
		// Exposure tone mapping for raw albedo
		Albedo = float3(1.0, 1.0, 1.0) - exp(-Albedo * Exposure);

		float3 N = MacrosurfaceNormal; // Macrosurface normal vector

		// This is equivalent of L vector (light direction from a point on interface-- both for macrosurface and microsurface.)
		float3 Wi_direct = DirectionalLightDirection.xyz;

		// This is equivalent of V vector (view direction from a point on interface-- both for macrosurface and microsurface.)
		float3 Wo = normalize(EyePosition.xyz - Input.WorldPosition.xyz);

		if (bUsePhysicallyBasedRendering)
		{
			// Calculate Fresnel reflectance at incident angle of 0 degree
			float3 F0 = lerp(KFresnel_dielectric, Albedo, BlendedMetalness);

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
				// L_o = ∫(BRDF() * L_i * cosθ)dω  <= Reflectance equation <= Rendering equation

				float3 Lo_direct_diff = (Kd_direct * DiffuseBRDF) * Li_direct * NdotWi_direct;
				float3 Lo_direct_spec = (Ks_direct * SpecularBRDF) * Li_direct * NdotWi_direct;

				OutputColor.xyz = Lo_direct_diff + Lo_direct_spec;
			}

			// Indirect light (only diffuse)
			{
				// Indirect light direction
				float3 Wi_indirect = normalize(-Wo + (2.0 * max(dot(N, Wo), 0) * N));

				// Radiance of indirect light
				float3 Ei_indirect = IrradianceTexture.Sample(TerrainSampler, Wi_indirect).rgb;

				float3 Lo_indirect_diff = Ei_indirect * Albedo;

				OutputColor.xyz += Lo_indirect_diff * BlendedAmbientOcclusion;
			}
		}
		else
		{
			float3 Ambient = CalculateClassicalAmbient(Albedo, AmbientLightColor, AmbientLightIntensity);
			float3 Directional = CalculateClassicalDirectional(Albedo, Albedo, MaterialSpecularExponent, BlendedSpecularIntensity,
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

	// For normal & tangent drawing
	if (Input.bUseVertexColor != 0)
	{
		return Input.Color;
	}

	if (bIsHighlitPixel == true)
	{
		OutputColor = HighlitAlbedo;
	}

	return OutputColor;
}