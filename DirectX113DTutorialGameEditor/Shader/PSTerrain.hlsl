#include "Terrain.hlsli"

SamplerState CurrentSampler : register(s0);

Texture2D Layer0DiffuseTexture : register(t0);
Texture2D Layer1DiffuseTexture : register(t1);
Texture2D Layer2DiffuseTexture : register(t2);
Texture2D Layer3DiffuseTexture : register(t3);
Texture2D Layer4DiffuseTexture : register(t4);

Texture2D Layer0NormalTexture : register(t5);
Texture2D Layer1NormalTexture : register(t6);
Texture2D Layer2NormalTexture : register(t7);
Texture2D Layer3NormalTexture : register(t8);
Texture2D Layer4NormalTexture : register(t9);

Texture2D MaskingTexture : register(t10);

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

float4 main(VS_OUTPUT input) : SV_TARGET
{
	float4 WorldMaskingPosition = float4(input.WorldPosition.x, 0, -input.WorldPosition.z, 1);
	float4 LocalMaskingPosition = mul(WorldMaskingPosition, InverseTerrainWorld);
	float4 MaskingSpacePosition = mul(LocalMaskingPosition, MaskingSpaceMatrix);
	float4 Masking = MaskingTexture.Sample(CurrentSampler, MaskingSpacePosition.xz);

	float4 DiffuseLayer0 = Layer0DiffuseTexture.Sample(CurrentSampler, input.UV.xy);
	float4 DiffuseLayer1 = Layer1DiffuseTexture.Sample(CurrentSampler, input.UV.xy);
	float4 DiffuseLayer2 = Layer2DiffuseTexture.Sample(CurrentSampler, input.UV.xy);
	float4 DiffuseLayer3 = Layer3DiffuseTexture.Sample(CurrentSampler, input.UV.xy);
	float4 DiffuseLayer4 = Layer4DiffuseTexture.Sample(CurrentSampler, input.UV.xy);

	float4 Albedo = DiffuseLayer0;
	Albedo.xyz = lerp(Albedo.xyz, DiffuseLayer1.xyz, Masking.r);
	Albedo.xyz = lerp(Albedo.xyz, DiffuseLayer2.xyz, Masking.g);
	Albedo.xyz = lerp(Albedo.xyz, DiffuseLayer3.xyz, Masking.b);
	Albedo.xyz = lerp(Albedo.xyz, DiffuseLayer4.xyz, Masking.a);
	
	float4 NormalLayer0 = normalize((Layer0NormalTexture.Sample(CurrentSampler, input.UV.xy) * 2.0f) - 1.0f);
	float4 NormalLayer1 = normalize((Layer1NormalTexture.Sample(CurrentSampler, input.UV.xy) * 2.0f) - 1.0f);
	float4 NormalLayer2 = normalize((Layer2NormalTexture.Sample(CurrentSampler, input.UV.xy) * 2.0f) - 1.0f);
	float4 NormalLayer3 = normalize((Layer3NormalTexture.Sample(CurrentSampler, input.UV.xy) * 2.0f) - 1.0f);
	float4 NormalLayer4 = normalize((Layer4NormalTexture.Sample(CurrentSampler, input.UV.xy) * 2.0f) - 1.0f);

	float3x3 TextureSpace = float3x3(input.WorldTangent.xyz, input.WorldBitangent.xyz, input.WorldNormal.xyz);
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
		
		float4 TerrainPosition = float4(input.WorldPosition.x, 0, input.WorldPosition.z, 1);
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

	// # Gamma correction
	Albedo.xyz = pow(Albedo.xyz, 2.0f);

	float4 ResultColor = Albedo;
	{
		float4 Ambient = CalculateAmbient(Albedo, AmbientLightColor, AmbientLightIntensity);
		float4 Directional = CalculateDirectional(Albedo, Albedo, 1, 0,
			DirectionalLightColor, DirectionalLightDirection, normalize(EyePosition - input.WorldPosition), normalize(input.WorldNormal));

		// Directional Light의 위치가 지평선에 가까워질수록 빛의 세기를 약하게 한다.
		float Dot = dot(DirectionalLightDirection, KUpDirection);
		Directional.xyz *= pow(Dot, 0.6f);

		ResultColor = Ambient + Directional;
	}

	// # Gamma correction
	ResultColor.xyz = pow(ResultColor.xyz, 0.5f);

	// For normal & tangent drawing
	if (input.bUseVertexColor != 0)
	{
		return input.Color;
	}

	if (bIsHighlitPixel == true)
	{
		ResultColor = HighlitAlbedo;
	}

	return ResultColor;
}