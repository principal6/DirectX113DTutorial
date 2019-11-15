#include "Terrain.hlsli"

SamplerState TerrainSampler : register(s0);

Texture2D Layer0DiffuseTexture : register(t0);
Texture2D Layer0NormalTexture : register(t1);
Texture2D Layer0OpacityTexture : register(t2);
Texture2D Layer0SpecularIntensityTexture : register(t3);
// Displacement texture slot

Texture2D Layer1DiffuseTexture : register(t5);
Texture2D Layer1NormalTexture : register(t6);
Texture2D Layer1OpacityTexture : register(t7);
Texture2D Layer1SpecularIntensityTexture : register(t8);
// Displacement texture slot

Texture2D Layer2DiffuseTexture : register(t10);
Texture2D Layer2NormalTexture : register(t11);
Texture2D Layer2OpacityTexture : register(t12);
Texture2D Layer2SpecularIntensityTexture : register(t13);
// Displacement texture slot

Texture2D Layer3DiffuseTexture : register(t15);
Texture2D Layer3NormalTexture : register(t16);
Texture2D Layer3OpacityTexture : register(t17);
Texture2D Layer3SpecularIntensityTexture : register(t18);
// Displacement texture slot

Texture2D Layer4DiffuseTexture : register(t20);
Texture2D Layer4NormalTexture : register(t21);
Texture2D Layer4OpacityTexture : register(t22);
Texture2D Layer4SpecularIntensityTexture : register(t23);
// Displacement texture slot

Texture2D MaskingTexture : register(t25);

cbuffer cbMaskingSpace : register(b0)
{
	float4x4 MaskingSpaceMatrix;
}

cbuffer cbLights : register(b1)
{
	float4	DirectionalLightDirection;
	float4	DirectionalLightColor;
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
	bool	bHasDiffuseTexture;

	bool	bHasNormalTexture;
	bool	bHasOpacityTexture;
	bool	bHasSpecularIntensityTexture;
	bool	Reserved;
}

float4 main(VS_OUTPUT Input) : SV_TARGET
{
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

	float4 OutputColor = BlendedDiffuse;
	{
		float4 Ambient = CalculateClassicalAmbient(BlendedDiffuse, AmbientLightColor, AmbientLightIntensity);
		float4 Directional = CalculateClassicalDirectional(BlendedDiffuse, float4(MaterialSpecularColor, 1.0), MaterialSpecularExponent, BlendedSpecularIntensity,
			DirectionalLightColor, DirectionalLightDirection, normalize(EyePosition - Input.WorldPosition), normalize(BlendedNormal));

		// Directional Light의 위치가 지평선에 가까워질수록 빛의 세기를 약하게 한다.
		float Dot = dot(DirectionalLightDirection, KUpDirection);
		Directional.xyz *= pow(Dot, 0.6f);

		OutputColor = Ambient + Directional;
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