#include "Terrain.hlsli"

SamplerState CurrentSampler : register(s0);

Texture2D Layer0DiffuseTexture : register(t0);
Texture2D Layer0NormalTexture : register(t1);
Texture2D Layer0OpacityTexture : register(t2);
// Dispacement texture slot

Texture2D Layer1DiffuseTexture : register(t4);
Texture2D Layer1NormalTexture : register(t5);
Texture2D Layer1OpacityTexture : register(t6);
// Dispacement texture slot

Texture2D Layer2DiffuseTexture : register(t8);
Texture2D Layer2NormalTexture : register(t9);
Texture2D Layer2OpacityTexture : register(t10);
// Dispacement texture slot

Texture2D Layer3DiffuseTexture : register(t12);
Texture2D Layer3NormalTexture : register(t13);
Texture2D Layer3OpacityTexture : register(t14);
// Dispacement texture slot

Texture2D Layer4DiffuseTexture : register(t16);
Texture2D Layer4NormalTexture : register(t17);
Texture2D Layer4OpacityTexture : register(t18);
// Dispacement texture slot

Texture2D MaskingTexture : register(t20);

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
	float3	MaterialAmbient;
	float	SpecularExponent;
	float3	MaterialDiffuse;
	float	SpecularIntensity;
	float3	MaterialSpecular;
	bool	bHasDiffuseTexture;

	bool	bHasNormalTexture;
	bool	bHasOpacityTexture;
	bool2	Pads2;
}

float4 main(VS_OUTPUT Input) : SV_TARGET
{
	float4 WorldVertexPosition = float4(Input.WorldPosition.x, 0, -Input.WorldPosition.z, 1);
	float4 LocalMaskingPosition = mul(WorldVertexPosition, InverseTerrainWorld);
	float4 MaskingSpacePosition = mul(LocalMaskingPosition, MaskingSpaceMatrix);
	float4 Masking = MaskingTexture.Sample(CurrentSampler, MaskingSpacePosition.xz);

	float2 TexCoord = Input.UV.xy;
	float4 DiffuseLayer0 = Layer0DiffuseTexture.Sample(CurrentSampler, TexCoord);
	float4 DiffuseLayer1 = Layer1DiffuseTexture.Sample(CurrentSampler, TexCoord);
	float4 DiffuseLayer2 = Layer2DiffuseTexture.Sample(CurrentSampler, TexCoord);
	float4 DiffuseLayer3 = Layer3DiffuseTexture.Sample(CurrentSampler, TexCoord);
	float4 DiffuseLayer4 = Layer4DiffuseTexture.Sample(CurrentSampler, TexCoord);

	float4 Albedo = DiffuseLayer0;
	Albedo.xyz = lerp(Albedo.xyz, DiffuseLayer1.xyz, Masking.r);
	Albedo.xyz = lerp(Albedo.xyz, DiffuseLayer2.xyz, Masking.g);
	Albedo.xyz = lerp(Albedo.xyz, DiffuseLayer3.xyz, Masking.b);
	Albedo.xyz = lerp(Albedo.xyz, DiffuseLayer4.xyz, Masking.a);
	
	float4 NormalLayer0 = normalize((Layer0NormalTexture.Sample(CurrentSampler, TexCoord) * 2.0f) - 1.0f);
	float4 NormalLayer1 = normalize((Layer1NormalTexture.Sample(CurrentSampler, TexCoord) * 2.0f) - 1.0f);
	float4 NormalLayer2 = normalize((Layer2NormalTexture.Sample(CurrentSampler, TexCoord) * 2.0f) - 1.0f);
	float4 NormalLayer3 = normalize((Layer3NormalTexture.Sample(CurrentSampler, TexCoord) * 2.0f) - 1.0f);
	float4 NormalLayer4 = normalize((Layer4NormalTexture.Sample(CurrentSampler, TexCoord) * 2.0f) - 1.0f);

	float3x3 TextureSpace = float3x3(Input.WorldTangent.xyz, Input.WorldBitangent.xyz, Input.WorldNormal.xyz);
	float4 ResultNormal;
	ResultNormal = NormalLayer0;
	ResultNormal.xyz = lerp(ResultNormal.xyz, NormalLayer1.xyz, Masking.r);
	ResultNormal.xyz = lerp(ResultNormal.xyz, NormalLayer2.xyz, Masking.g);
	ResultNormal.xyz = lerp(ResultNormal.xyz, NormalLayer3.xyz, Masking.b);
	ResultNormal.xyz = lerp(ResultNormal.xyz, NormalLayer4.xyz, Masking.a);
	ResultNormal = normalize(ResultNormal);
	ResultNormal = normalize(float4(mul(ResultNormal.xyz, TextureSpace), 0.0f));
	
	// Selection highlight (for edit mode)
	bool bIsHighlitPixel = false;
	float4 HighlitAlbedo = Albedo;
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
			Albedo.xyz = max(Albedo.xyz, ColorCmp);
			Albedo.xyz += HighlightFactor * Sine;
			bIsHighlitPixel = true;
			HighlitAlbedo = Albedo;
		}
	}

	// # Here we make sure that input RGB values are in linear-space!
	// (In this project, all textures are loaded with their values in gamma-space)
	if (bHasDiffuseTexture == true)
	{
		// # Convert gamma-space RGB to linear-space RGB (sRGB)
		Albedo.xyz = pow(Albedo.xyz, 2.2);
	}

	float4 OutputColor = Albedo;
	{
		float4 Ambient = CalculateAmbient(Albedo, AmbientLightColor, AmbientLightIntensity);
		float4 Directional = CalculateDirectional(Albedo, float4(MaterialSpecular, 1.0), SpecularExponent, SpecularIntensity,
			DirectionalLightColor, DirectionalLightDirection, normalize(EyePosition - Input.WorldPosition), normalize(ResultNormal));

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