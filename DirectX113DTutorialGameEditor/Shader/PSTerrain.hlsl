#include "Terrain.hlsli"
#include "BRDF.hlsli"

SamplerState TerrainSampler : register(s0);

Texture2D Layer0DiffuseTexture : register(t0);
Texture2D Layer0NormalTexture : register(t1);
Texture2D Layer0OpacityTexture : register(t2);
Texture2D Layer0SpecularIntensityTexture : register(t3);
Texture2D Layer0RoughnessTexture : register(t4);
Texture2D Layer0MetalnessTexture : register(t5);
// Displacement texture slot

Texture2D Layer1DiffuseTexture : register(t7);
Texture2D Layer1NormalTexture : register(t8);
Texture2D Layer1OpacityTexture : register(t9);
Texture2D Layer1SpecularIntensityTexture : register(t10);
Texture2D Layer1RoughnessTexture : register(t11);
Texture2D Layer1MetalnessTexture : register(t12);
// Displacement texture slot

Texture2D Layer2DiffuseTexture : register(t14);
Texture2D Layer2NormalTexture : register(t15);
Texture2D Layer2OpacityTexture : register(t16);
Texture2D Layer2SpecularIntensityTexture : register(t17);
Texture2D Layer2RoughnessTexture : register(t18);
Texture2D Layer2MetalnessTexture : register(t19);
// Displacement texture slot

Texture2D Layer3DiffuseTexture : register(t21);
Texture2D Layer3NormalTexture : register(t22);
Texture2D Layer3OpacityTexture : register(t23);
Texture2D Layer3SpecularIntensityTexture : register(t24);
Texture2D Layer3RoughnessTexture : register(t25);
Texture2D Layer3MetalnessTexture : register(t26);
// Displacement texture slot

Texture2D Layer4DiffuseTexture : register(t28);
Texture2D Layer4NormalTexture : register(t29);
Texture2D Layer4OpacityTexture : register(t30);
Texture2D Layer4SpecularIntensityTexture : register(t31);
Texture2D Layer4RoughnessTexture : register(t32);
Texture2D Layer4MetalnessTexture : register(t33);
// Displacement texture slot

Texture2D MaskingTexture : register(t35);

cbuffer cbMaskingSpace : register(b0)
{
	float4x4 MaskingSpaceMatrix;
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

cbuffer cbSelection : register(b2)
{
	bool bShowSelection;
	float SelectionRadius;
	float2 AnaloguePosition;

	float4x4 TerrainWorld;
	float4x4 InverseTerrainWorld;
}

cbuffer cbEditorTime : register(b3)
{
	float NormalizedTime;
	float NormalizedTimeHalfSpeed;
	float2 Pad2;
}

cbuffer cbMaterial : register(b4)
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
	float SpecularIntensity = MaterialSpecularIntensity;
	float Roughness = MaterialRoughness;
	float Metalness = MaterialMetalness;

	float4 WorldVertexPosition = float4(Input.WorldPosition.x, 0, -Input.WorldPosition.z, 1);
	float4 LocalMaskingPosition = mul(WorldVertexPosition, InverseTerrainWorld);
	float4 MaskingSpacePosition = mul(LocalMaskingPosition, MaskingSpaceMatrix);
	float4 Masking = MaskingTexture.Sample(TerrainSampler, MaskingSpacePosition.xz);

	const float2 KTexCoord = Input.UV.xy;

	float4 DiffuseLayer0 = Layer0DiffuseTexture.Sample(TerrainSampler, KTexCoord);
	float4 DiffuseLayer1 = Layer1DiffuseTexture.Sample(TerrainSampler, KTexCoord);
	float4 DiffuseLayer2 = Layer2DiffuseTexture.Sample(TerrainSampler, KTexCoord);
	float4 DiffuseLayer3 = Layer3DiffuseTexture.Sample(TerrainSampler, KTexCoord);
	float4 DiffuseLayer4 = Layer4DiffuseTexture.Sample(TerrainSampler, KTexCoord);

	float4 BlendedDiffuse = float4(MaterialDiffuseColor, 1);
	if (bHasDiffuseTexture == true)
	{
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
	if (bHasNormalTexture)
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
	if (bHasSpecularIntensityTexture)
	{
		BlendedSpecularIntensity = SpecularIntensityLayer0;
		BlendedSpecularIntensity = lerp(BlendedSpecularIntensity, SpecularIntensityLayer1, Masking.r);
		BlendedSpecularIntensity = lerp(BlendedSpecularIntensity, SpecularIntensityLayer2, Masking.g);
		BlendedSpecularIntensity = lerp(BlendedSpecularIntensity, SpecularIntensityLayer3, Masking.b);
		BlendedSpecularIntensity = lerp(BlendedSpecularIntensity, SpecularIntensityLayer4, Masking.a);
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

	// # Here we make sure that input RGB values are in linear-space!
	// (In this project, all textures are loaded with their values in gamma-space)
	if (bHasDiffuseTexture == true)
	{
		// # Convert gamma-space RGB to linear-space RGB (sRGB)
		BlendedDiffuse.xyz = pow(BlendedDiffuse.xyz, 2.2);
	}


	float3 MacrosurfaceNormal = BlendedNormal.xyz;
	float3 BaseColor = BlendedDiffuse.xyz;
	float4 OutputColor = float4(BaseColor, 1);
	{
		// Exposure tone mapping
		BaseColor = float3(1.0, 1.0, 1.0) - exp(-BaseColor * Exposure);

		float3 N = MacrosurfaceNormal;
		float3 L = DirectionalLightDirection.xyz;
		float3 V = normalize(EyePosition.xyz - Input.WorldPosition.xyz);

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

	/*
	float4 OutputColor = BlendedDiffuse;
	{
		float3 N = normalize(BlendedNormal).xyz;
		float3 L = DirectionalLightDirection.xyz;
		float3 V = normalize(EyePosition.xyz - Input.WorldPosition.xyz);

		float3 Ambient = CalculateClassicalAmbient(BlendedDiffuse.xyz, AmbientLightColor, AmbientLightIntensity);
		float3 Directional = CalculateClassicalDirectional(BlendedDiffuse.xyz, MaterialSpecularColor, MaterialSpecularExponent, BlendedSpecularIntensity,
			DirectionalLightColor, L, V, N);

		// Directional Light의 위치가 지평선에 가까워질수록 빛의 세기를 약하게 한다.
		float Dot = dot(DirectionalLightDirection, KUpDirection);
		Directional.xyz *= pow(Dot, 0.6f);

		OutputColor.xyz = Ambient + Directional;
	}
	*/

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